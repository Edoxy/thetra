#include "GenericDomain.hpp"
#include "GenericMesh.hpp"
#include "Domain3D.hpp"
#include "MeshImport_Tetgen.hpp"
#include "RefinerTetra.hpp"
#include "Cutter3D.hpp"
#include <iostream>

using namespace GeDiM;
using namespace Eigen;

int main(int argc, char** argv)
{
	/// CREATE DOMAIN
	unsigned int id = 1;

    Parallelepiped domain(id);
    Vector3d origin(1.0, 1.0, 1.0), length(1.0, 0.0, 0.0), height(0.0, 0.0, 1.0), width(0.0, 1.0, 0.0);

    domain.BuildParallelepiped(origin, length, height, width);

    MeshImport_Tetgen meshImportTetgen;
    meshImportTetgen.SetMinimumNumberOfCells(1);
    GenericMesh mesh;
    meshImportTetgen.CreateTetgenInput(domain);
    meshImportTetgen.CreateTetgenOutput(domain);
    meshImportTetgen.CreateMesh(domain, mesh);

    cout << mesh.NumberOfCells() << endl;
    cout << mesh.NumberOfFaces() << endl;

    for(int i = 0; i < mesh.NumberOfCells(); i++)
    {
        cout << mesh.Cell(i)->Id() << " ";
    }

	/// REFINE MESH
	unsigned int numCellToRefiner = 1;
	RefinerTetra refiner;
	refiner.SetMesh(mesh);
	refiner.InitializeIdCells(numCellToRefiner);
	for(unsigned int numCell = 0; numCell < numCellToRefiner; numCell++)
        refiner.AddIdCell(numCell);
    refiner.RefineMesh();
	/// OUTPUT MESH TO MATLAB SCRIPT FOR VISUALIZATION

	cout << mesh.NumberOfCells() << endl;
    cout << mesh.NumberOfFaces() << endl;

	mesh.CleanInactiveTreeNode();
	ofstream file("plotTetrahedralMesh.m", ofstream::out );
	if(file.is_open())
	{
        file << "nodes = [";
        for(unsigned int i = 0; i < mesh.NumberOfPoints(); i++)
            file << mesh.Point(i)->Coordinates()(0) << "," <<  mesh.Point(i)->Coordinates()(1) << "," <<  mesh.Point(i)->Coordinates()(2) << ";" << endl;
        file << "];" << endl;

        file << "connectivity = [";
        for(unsigned int i = 0; i < mesh.NumberOfCells(); i++)
            {
                file << mesh.Cell(i)->Point(0)->Id()+1 << "," <<  mesh.Cell(i)->Point(1)->Id()+1 << "," << mesh.Cell(i)->Point(2)->Id()+1 << "," << mesh.Cell(i)->Point(3)->Id()+1 << ";" << endl;
            }
        file << "];" << endl;
        file << "tetramesh(connectivity, nodes);" << endl;
        file.close();
	}
	else
        Output::PrintErrorMessage("Unable to open the file", true);

}
