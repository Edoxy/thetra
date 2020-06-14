#ifndef REFINERTETRA_HPP
#define REFINERTETRA_HPP

#include "GenericMesh.hpp"
#include "vector"
#include "list"
#include "Output.hpp"

namespace GeDiM
{
    class RefinerTetra
    {
        protected:

            GenericMesh* meshPointer;
            vector<unsigned int> idCellToRefine;
            list<unsigned int> idFacesCut;

            const Output::ExitCodes FindMaxFace(GenericCell& cell) {}
            const Output::ExitCodes CutTetra(GenericCell& cell) {}
            const Output::ExitCodes RecoverConformity() {}
            const GenericEdge* FindMaxEdge(GenericCell& cell);


        public:
            RefinerTetra();
            virtual ~RefinerTetra();

            void SetMesh(GenericMesh& mesh) {meshPointer = &mesh;}
            const Output::ExitCodes InitializeIdCells(const unsigned int& numberOfCells) { idCellToRefine.reserve(numberOfCells); return Output::Success;}
            const Output::ExitCodes	AddIdCell(const unsigned int& idCell);
            const Output::ExitCodes RefineMesh(){}
    };
}

#endif // REFINERTETRA_H
