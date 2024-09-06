//single-rocess and multi-rocess rendering functions(ray tracing part is not here)
#include <algorithm>
#include <vector>
#include <mpi.h>
#include <chrono>
#include <iomanip>

#include "SceneParser.hpp"
#include "Image.hpp"
#include "Camera.hpp"
#include "Group.hpp"
#include "Light.hpp"
#include "MCTracer.hpp"			//<-- this is the Monte Carlo ray tracing part
#include "Configuration.hpp"

using namespace std;

void printRenderInforation(const SceneParser& sceneParser)
{
	cout << "--- Render Information ---" << endl;
	cout << "- input filepath    | " << getInputFilePath(inputFiles[CHOICE]) << endl;
	cout << "- output filepath   | " << getOutputFilePath(outputFiles[CHOICE]) << endl;
	cout << "- image resolution  | " << WIDTH << " x " << HEIGHT << endl;
	cout << "- MPI acceleration  | " << (USEMPI ? "true" : "false") << endl;
	cout << "- supersampling     | " << (SUPERSAMPLING ? "true" : "false") << endl;
	cout << "- jittored sampling | " << (JITTER ? "true" : "false") << endl;
	cout << "- Gaussian blur     | " << (GAUSSIANBLUR ? "true" : "false") << endl;
	if (sceneParser.checkStatus())
	{
		cout << "- # objects         | " << sceneParser.getNumObjects() << endl;
		cout << "- # lights          | " << sceneParser.getNumLights() << endl;
		cout << "- # light objects   | " << sceneParser.getNumLightObjects() << endl;
		cout << "- # materials       | " << sceneParser.getNumMaterials() << endl;
		cout << "- is stochastic     | " << (sceneParser.hasStochasticScene() || sceneParser.hasStochasticCamera() ||  JITTER ? "true" : "false") << endl;
		cout << "- ready to start rendering" << endl << endl;
	}
	else
	{
		cout << "- sceneparser error | " << sceneParser.getErrorMessage() << endl;
		cout << "- abort program" << endl << endl;
	}
}

//single-process rendering
void render()
{
	auto start = chrono::high_resolution_clock::now();

	//render size(not output size)
	int width = SUPERSAMPLING ? (WIDTH * 3) : WIDTH;
	int height = SUPERSAMPLING ? (HEIGHT * 3) : HEIGHT;

	SceneParser sceneParser(inputFiles[CHOICE]);
	printRenderInforation(sceneParser);
	if (!sceneParser.checkStatus())
		return;
	//for static scene, no need to repeat computation
	bool needRegenerateRay = sceneParser.hasStochasticCamera();
	int sampleRate = sceneParser.hasStochasticScene() || needRegenerateRay || JITTER ? SAMPLERATE : 1;
	
	Camera* camera = sceneParser.getCamera();
	camera->setSize(width, height);

	Image img(width, height);

	MCTracer tracer(&sceneParser, 1.0);

	int tenth = (width + 9) / 10;

	for (int i = 0; i < width; i++)
	{
		if (i%tenth == 0)
		{
			cout << "render column " << setw(4) << i << " / " << setw(4) << width << endl;
		}
		for (int j = 0; j < height; j++)
		{
			Vector3f color;
			Ray ray = camera->generateRay(i, j);
			for (int k = 0; k < sampleRate; k++)
			{
				if (JITTER)
					ray = camera->generateJittoredRay(i, j);
				else if (needRegenerateRay)
					ray = camera->generateRay(i, j);

				Hit hit;
				Vector3f result = tracer.traceRay(ray, hit);
				if (isnan(result[0]))
					continue;
				color = color + result;
			}
			color = color / sampleRate;

			img.SetPixel(i, j, color);
		}
	}

	if (GAUSSIANBLUR)
	{
		cout << "- start Gaussian blurring" << endl;
		img.GaussianBlur();
	}

	if (SUPERSAMPLING)
	{
		Image small_img(WIDTH, HEIGHT);
		cout << "- start down sampling" << endl;
		small_img.DownSampling(img);
		small_img.SaveImage(getOutputFilePath(outputFiles[CHOICE]).c_str());
	}
	else
		img.SaveImage(getOutputFilePath(outputFiles[CHOICE]).c_str());

	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double> diff = end - start;

	int total_seconds = static_cast<int>(diff.count());
	int hours = total_seconds / 3600;
	total_seconds %= 3600;
	int minutes = total_seconds / 60;
	int seconds = total_seconds % 60;

	cout << "- maximum recursion depth | " << tracer.maximumDepth() << endl;
	cout << "- elapsed time            | " << hours << ":" << minutes << ":" << seconds << endl;
}

//sort pairs indescending order
bool compare(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
	return a.first > b.first;
}

//multi-process rendering
void render_MPI(int argc, char* argv[])
{
	//##################################################################
	//							Preparation
	//##################################################################
	auto start = chrono::high_resolution_clock::now();

	MPI_Init(&argc, &argv);

	int MPI_size;
	int MPI_rank;
	MPI_Comm_size(MPI_COMM_WORLD, &MPI_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &MPI_rank);

	int width = SUPERSAMPLING ? (WIDTH * 3) : WIDTH;
	int height = SUPERSAMPLING ? (HEIGHT * 3) : HEIGHT;

	SceneParser sceneParser(inputFiles[CHOICE]);
	//log some information
	if (MPI_rank == 0)
	{
		printRenderInforation(sceneParser);
	}
	if (!sceneParser.checkStatus())
	{
		MPI_Finalize();
		return;
	}
	//for static scene, no need to repeat computation
	bool needRegenerateRay = sceneParser.hasStochasticCamera();
	int sampleRate = sceneParser.hasStochasticScene() || needRegenerateRay || JITTER ? SAMPLERATE : 1;

	Camera* camera = sceneParser.getCamera();
	camera->setSize(width, height);

	//RayTracer tracer(&sceneParser, 0, 1.0);
	MCTracer tracer(&sceneParser, 1.0);

	MPI_Barrier(MPI_COMM_WORLD);

	//##################################################################
	//							Scheduling
	//##################################################################

	//what to do in rendering
	vector<int> column2do;

	//only useful for process 0
	vector<vector<int>> schedule;

	if (sampleRate > 1)
	{
		if (MPI_rank == 0)
			cout << "- start process scheduling" << endl;

		//each process do some measurements, then gather them together
		double* measurement = new double[(width + MPI_size - 1) / MPI_size];
		int blockSize = (width + MPI_size - 1) / MPI_size;
		for (int i = 0; i < blockSize; i++)
			measurement[i] = -1;

		//estimate time consumption
		for (int i = MPI_rank; i < width; i += MPI_size)
		{
			auto clock1 = chrono::high_resolution_clock::now();
			for (int j = 0; j < height; j++)
			{
				Vector3f color;
				Ray ray = camera->generateRay(i, j);
				for (int k = 0; k < 3; k++)
				{
					if (JITTER)
						ray = camera->generateJittoredRay(i, j);
					else if (needRegenerateRay)
						ray = camera->generateRay(i, j);

					Hit hit;
					Vector3f result = tracer.traceRay(ray, hit);
					color = color + result;
				}
				//no need to store result
			}
			auto clock2 = chrono::high_resolution_clock::now();
			chrono::duration<double, milli> diff = clock2 - clock1;
			measurement[i / MPI_size] = diff.count();
		}

		//gather measuerments and compute schedule
		if (MPI_rank == 0)
		{
			//gather measurements
			vector<pair<double, int>> timePerColumn;

			//measurements of process 0
			for (int i = 0; i < blockSize; i++)
			{
				if (measurement[i] == -1)
					break;
				else
					timePerColumn.push_back(make_pair(measurement[i], i * MPI_size));
			}

			//measurements from other processes
			MPI_Status status;
			for (int source = 1; source < MPI_size; source++)
			{
				MPI_Recv(measurement, blockSize, MPI_DOUBLE, source, 0, MPI_COMM_WORLD, &status);
				for (int i = 0; i < blockSize; i++)
				{
					if (measurement[i] == -1)
						break;
					else
						timePerColumn.push_back(make_pair(measurement[i], i * MPI_size + source));
				}
			}

			//start scheduling
			vector<double> sums;
			for (int i = 0; i < MPI_size; i++)
			{
				vector<int> V;
				schedule.push_back(V);
				sums.push_back(0);
			}

			//sort in descending order
			sort(timePerColumn.begin(), timePerColumn.end(), compare);

			for (int i = 0; i < width; i++)
			{
				//choose most vacant process
				int process = distance(sums.begin(), min_element(sums.begin(), sums.end()));
				//casher's algorithm
				schedule[process].push_back(timePerColumn[i].second);
				sums[process] += timePerColumn[i].first;
			}

			//schedule for process 0 itself
			for (auto column : schedule[0])
				column2do.push_back(column);

			int* package = new int[width]; //"width" is the upper bound of package size
			for (int target = 1; target < MPI_size; target++)
			{
				//make a package for each process
				int workSize = schedule[target].size();
				for (int j = 0; j < width; j++)
				{
					if (j < workSize)
						package[j] = schedule[target][j];
					else
						package[j] = -1;
				}
				MPI_Send(package, width, MPI_INT, target, 0, MPI_COMM_WORLD);
			}

			delete[] package;
		}
		else
		{
			MPI_Send(measurement, blockSize, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

			//receive assignments from process 0
			MPI_Status status;
			int* package = new int[width];
			MPI_Recv(package, width, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

			for (int i = 0; i < width; i++)
			{
				if (package[i] == -1)
					break;
				else
					column2do.push_back(package[i]);
			}

			delete[] package;
		}

		delete[] measurement;

		if (MPI_rank == 0)
		{
			cout << "- finish process scheduling" << endl;
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
	else
	{
		for (int column = MPI_rank; column < width; column += MPI_size)
			column2do.push_back(column);

		if (MPI_rank == 0)
		{
			for (int process = 0; process < MPI_size; process++)
			{
				vector<int> work;
				for (int column = process; column < width; column += MPI_size)
					work.push_back(column);

				schedule.push_back(work);
			}
		}
	}

	//##################################################################
	//							Computation
	//##################################################################
	
	//store RBG values
	float* data = new float[column2do.size() * height * 3];

	int tenth = (column2do.size() + 9) / 10;

	for (int i = 0; i < column2do.size(); i++)
	{
		if (i % tenth == 0)
		{
			cout << "Process " << setw(2) << MPI_rank << " render column " << setw(4) << i << " / " << setw(4) << column2do.size() << endl;
		}

		for (int j = 0; j < height; j++)
		{
			Vector3f color;
			Ray ray = camera->generateRay(column2do[i], j);
			for (int k = 0; k < sampleRate; k++)
			{
				//Generate ray
				if (JITTER)
					ray = camera->generateJittoredRay(column2do[i], j);
				else if (needRegenerateRay)
					ray = camera->generateRay(column2do[i], j);

				Hit hit;
				Vector3f result = tracer.traceRay(ray, hit);
				if (isnan(result[0]))
					continue;
				color = color + result;
			}
			color = color / sampleRate;

			data[i * height * 3 + j * 3] = color[0];
			data[i * height * 3 + j * 3 + 1] = color[1];
			data[i * height * 3 + j * 3 + 2] = color[2];
		}
	}

	//merge into a whole picture
	MPI_Status recv_status;
	if (MPI_rank == 0)
	{
		Image img(width, height);
		//piece rendered by process 0
		for (int i = 0; i < column2do.size(); i++)
		{
			for (int j = 0; j < height; j++)
			{
				img.SetPixel(column2do[i], j, Vector3f(data[i * height * 3 + j * 3], data[i * height * 3 + j * 3 + 1], data[i * height * 3 + j * 3 + 2]));
			}
		}

		//gather image pieces from other processes
		MPI_Status recv_status;
		float* package = new float[height * 3];
		for (int source = 1; source < MPI_size; source++)
		{
			//transfer one column at a time
			for (int i = 0; i < schedule[source].size(); i++)
			{
				MPI_Recv(package, height * 3, MPI_FLOAT, source, i, MPI_COMM_WORLD, &recv_status);

				for (int j = 0; j < height; j++)
				{
					img.SetPixel(schedule[source][i], j, Vector3f(package[j * 3], package[j * 3 + 1], package[j * 3 + 2]));
				}
			}
		}
		delete[] package;

		if (GAUSSIANBLUR)
		{
			cout << "- start Gaussian blurring" << endl;
			img.GaussianBlur();
		}

		if (SUPERSAMPLING)
		{
			Image small_img(WIDTH, HEIGHT);
			cout << "- start down sampling" << endl;
			small_img.DownSampling(img);
			small_img.SaveImage(getOutputFilePath(outputFiles[CHOICE]).c_str());
		}
		else
			img.SaveImage(getOutputFilePath(outputFiles[CHOICE]).c_str());
		
	}
	else
	{
		//send result to process one at a time
		for (int i = 0; i < column2do.size(); i++)
		{
			MPI_Send(&(data[i * height * 3]), height * 3, MPI_FLOAT, 0, i, MPI_COMM_WORLD);
		}
	}

	delete[] data;

	MPI_Finalize();

	auto end = chrono::high_resolution_clock::now();
	chrono::duration<double> diff = end - start;

	// Only process 0 prints the elapsed time
	if (MPI_rank == 0)
	{
		int total_seconds = static_cast<int>(diff.count());
		int hours = total_seconds / 3600;
		total_seconds %= 3600;
		int minutes = total_seconds / 60;
		int seconds = total_seconds % 60;

		cout << "- maximum trace depth | " << tracer.maximumDepth() << endl;
		cout << "- elapsed time        | " << hours << ":" << minutes << ":" << seconds << endl;
	}
}
