//there are several ways to run this code:
//1. hit "start" button
//2. run "mpiexec -n 16 .\Graphics" inside "Graphics\x64\release" directory
// (you need to set "USEMPI" as true in "Configuration.hpp")
#include "Render.hpp"

int main(int argc, char* argv[])
{
	if (USEMPI)
		render_MPI(argc, argv);
	else
		render();

	return 0;
}