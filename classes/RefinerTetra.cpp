#include "RefinerTetra.hpp"
#include "Output.hpp"
#include "Cutter3D.hpp"

using namespace MainApplication;

namespace GeDiM
{
    RefinerTetra::RefinerTetra()
    {

    }
    RefinerTetra::~RefinerTetra()
    {

    }
    const Output::ExitCodes RefinerTetra::AddIdCell(const unsigned int& idCell)
	{
		idCellToRefine.push_back(idCell);
		return Output::Success;
	}

    const GenericEdge* RefinerTetra::FindMaxEdge(GenericCell& cell)
    {
        //ricerca lato più lungo
        double max = 0;
        unsigned int edge_pos;
        for(int i = 0; i < cell.NumberOfEdges(); i++)
        {
            const Vector3d first_point = cell.Edge(i) -> Point(0) -> Coordinates();
            const Vector3d second_point = cell.Edge(i) -> Point(1) -> Coordinates();
            double length = (second_point - first_point).norm();
            if(max < length)
            {
                max = length;
                edge_pos = i;
            }
        }
        return cell.Edge(edge_pos);
    }

    const Output::ExitCodes RefinerTetra::CutTetra(GenericCell& cell)
    {
        //Controllo che non sia già stato tagliato
        if(cell.IsActive())
        {
            //cerco il lato da tagliare
            const GenericEdge* edge_to_split = this -> FindMaxEdge(cell);
            //trovo il punto medio
            GenericPoint* new_point = meshPointer -> CreatePoint();
            Vector3d point = 0.5 * (edge_to_split -> Point(0) ->Coordinates() + edge_to_split -> Point(1) ->Coordinates());
            new_point -> SetCoordinates(point);


            CutterMesh3D cutter;
            cutter.intersector2D1D = new Intersector2D1D;
            //compute normal
            GenericFace* cutter_plane = meshPointer -> CreateFace();
            cutter_plane -> AddPoint(new_point);
            for(int i = 0; i < cell.NumberOfPoints() ; i++)
            {
                const GenericPoint* current_point = meshPointer->Point(i);
                if(current_point != edge_to_split->Point(0) && current_point != edge_to_split->Point(1))
                {
                    cutter_plane->AddPoint(current_point);
                }
            }
            cutter_plane->ComputeNormal();
            cutter.CutCell(cell, cutter_plane->Normal(), 30);

            //cutter.CutCell();
        }
        return Output::Success;
    }

    const Output::ExitCodes RefinerTetra::RecoverConformity()
    {

    }

    const Output::ExitCodes RefinerTetra::RefineMesh()
    {
        for(int i = 0; i < idCellToRefine.size(); i++)
        {
            this -> CutTetra(*meshPointer -> Cell(idCellToRefine[i]));
            this -> RecoverConformity();


        }
        return Output::Success;
    }
}
