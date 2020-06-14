#include "RefinerTetra.hpp"
#include "Output.hpp"

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

    const Output::ExitCodes RefinerTetra::RefineMesh()
    {

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
            const GenericEdge* edge_to_split = this -> FindMaxEdge(cell);
            
        }
    }

    const Output::ExitCodes RefinerTetra::RecoverConformity()
    {

    }
}
