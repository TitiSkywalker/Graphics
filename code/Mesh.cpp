#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <sstream>

#include "Mesh.hpp"

using namespace std;

//accelerator will call this function to check triangle intersection
static void intersectCall(int idx, void** arg)
{
	Mesh* m = (Mesh*)(arg[0]);
	bool result = m->intersectTrig(idx);
	arg[1] = (void*)(((bool)arg[1]) | result);
}

bool Mesh::intersect(const Ray& r, Hit& h, float tm)
{
	//store it so that "Mesh::intersectTrig" can use it
	ray = &r;
	hit = &h;
	tmin = tm;

	//how to interact with accelerator? pass self as argument
	void* arg[2]{};
	arg[0] = this;
	arg[1] = 0;

	//accelerator reads arg and uses arg[0] to construct and intersect
	//accelerator shares "arg" with "Mesh"
	hierarchy.arg = arg;
	hierarchy.termFunc = intersectCall;
	hierarchy.intersect(r);
	return arg[1];
}

//intersect a triangle at location "idx"
bool Mesh::intersectTrig(int idx) 
{
	Triangle triangle(v[t[idx][0]], v[t[idx][1]], v[t[idx][2]], material);

	//compute normals
	if (autoNormal)
	{
		if (smooth)
		{
			triangle.normals[0] = n[t[idx][0]];
			triangle.normals[1] = n[t[idx][1]];
			triangle.normals[2] = n[t[idx][2]];
		}
		else
		{
			triangle.normals[0] = n[idx];
			triangle.normals[1] = n[idx];
			triangle.normals[2] = n[idx];
		}
	}
	else
	{
		triangle.normals[0] = n[t[idx].texORnormID[0]];
		triangle.normals[1] = n[t[idx].texORnormID[1]];
		triangle.normals[2] = n[t[idx].texORnormID[2]];
	}

	if (hasTexture) 
	{
		triangle.texCoords[0] = texCoord[t[idx].texORnormID[0]];
		triangle.texCoords[1] = texCoord[t[idx].texORnormID[1]];
		triangle.texCoords[2] = texCoord[t[idx].texORnormID[2]];
		triangle.hasTex = true;
	}

	return triangle.intersect(*ray, *hit, tmin);
}

//parse .obj file
Mesh::Mesh(const char* filename, Material* material): Object3D(material)
{
	ray = NULL;
	hit = NULL;
	tmin = 0;

	smooth = false;
	autoNormal = false;
	hasTexture = false;

	ifstream f;
	f.open(filename);
	if (!f.is_open())
		throw runtime_error("cannot open mesh file " + string(filename));

	string line;
	string vTok("v");		//vertex
	string fTok("f");		//face
	string texTok("vt");	//texture
	string normTok("vn");	// normal

	char bslash = '/', space = ' ';
	string tok;

	while (1) 
	{
		std::getline(f, line);
		if (f.eof()) 
		{
			//finish reading
			break;
		}
		if (line.size() < 3) 
		{
			//empty line
			continue;
		}
		if (line.at(0) == '#') 
		{
			//notation
			continue;
		}
		stringstream ss(line);
		ss >> tok;
		if (tok == vTok)
		{
			//define a vertex 3D coordinate
			Vector3f vertex;
			ss >> vertex[0] >> vertex[1] >> vertex[2];
			v.push_back(vertex);
		}
		else if (tok == fTok)
		{
			// define a face (triangle or quad)
			vector<int> vertices, texORnormIDs;

			if (line.find(bslash) != string::npos)
			{
				//format: f vertexid//normalid ... or f vertexid/textureid ...
				//bind vertex with corresponding normal
				replace(line.begin(), line.end(), bslash, space);
				stringstream facess(line);

				facess >> tok;
				int vertex, texORnormal;
				while (facess >> vertex >> texORnormal)
				{
					vertices.push_back(vertex);
					texORnormIDs.push_back(texORnormal);
				}
				if (vertices.size() == 3)	//triangle
				{
					Trig trig;
					trig[0] = vertices[0] - 1;
					trig[1] = vertices[1] - 1;
					trig[2] = vertices[2] - 1;
					trig.texORnormID[0] = texORnormIDs[0] - 1;
					trig.texORnormID[1] = texORnormIDs[1] - 1;
					trig.texORnormID[2] = texORnormIDs[2] - 1;
					t.push_back(trig);
				}
				else	//quad
				{
					Trig trig1, trig2;
					trig1[0] = vertices[0] - 1;
					trig1[1] = vertices[1] - 1;
					trig1[2] = vertices[2] - 1;
					trig1.texORnormID[0] = texORnormIDs[0] - 1;
					trig1.texORnormID[1] = texORnormIDs[1] - 1;
					trig1.texORnormID[2] = texORnormIDs[2] - 1;
					t.push_back(trig1);
					trig2[0] = vertices[0] - 1;
					trig2[1] = vertices[2] - 1;
					trig2[2] = vertices[3] - 1;
					trig2.texORnormID[0] = texORnormIDs[0] - 1;
					trig2.texORnormID[1] = texORnormIDs[2] - 1;
					trig2.texORnormID[2] = texORnormIDs[3] - 1;
					t.push_back(trig2);
				}
			}
			else 
			{
				// just a face with 3 vertices
				Trig trig;
				ss >> trig[0] >> trig[1] >> trig[2];
				trig[0]--; trig[1]--; trig[2]--;
				t.push_back(trig);
			}
		}
		else if (tok == texTok)
		{
			//define a texture coordinate
			Vector2f texcoord;
			ss >> texcoord[0] >> texcoord[1];
			texCoord.push_back(texcoord);
		}
		else if (tok == normTok) 
		{
			//define a normal
			Vector3f norm;
			ss >> norm[0] >> norm[1] >> norm[2];
			n.push_back(norm);
		}
	}
	f.close();

	if (n.size() == 0)
	{
		//no normal specified, automatically compute normals
		autoNormal = true;

		if (t.size() > 200)
			smooth = true;
		else
			smooth = false;
		computeNorm();
	}

	if (texCoord.size() > 0)
		hasTexture = true;

	hierarchy.build(*this);
}

//compute normal for each vertex
void Mesh::computeNorm()
{
	if (smooth) 
	{
		//need to store normals in 3 vertices, then interpolate
		n.resize(v.size());
		for (unsigned int ii = 0; ii < t.size(); ii++) 
		{
			//get 2 edge vectors
			Vector3f a = v[t[ii][1]] - v[t[ii][0]];
			Vector3f b = v[t[ii][2]] - v[t[ii][0]];
			b = Vector3f::cross(a, b);
			for (int dim = 0; dim < 3; dim++) 
			{
				//the value of b is related to triangle's area
				//each triangle affects 3 vertices
				n[t[ii][dim]] += b;
			}
		}
		for (unsigned int ii = 0; ii < v.size(); ii++) 
		{
			//consider all adjacent triangles of a vertex, then normalize
			n[ii] = n[ii] / n[ii].length();
		}
	}
	else 
	{
		//each triangle has a fixed normal
		n.resize(t.size());
		for (unsigned int ii = 0; ii < t.size(); ii++) 
		{
			//get 2 edge vectors
			Vector3f a = v[t[ii][1]] - v[t[ii][0]];
			Vector3f b = v[t[ii][2]] - v[t[ii][0]];
			b = Vector3f::cross(a, b);
			n[ii] = b.normalized();
		}
	}
}
