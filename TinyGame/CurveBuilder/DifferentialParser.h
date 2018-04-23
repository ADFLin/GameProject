#ifndef DifferentialParser_H
#define DifferentialParser_H

#include "FunctionParser.h"


struct ExprTreeData
{
	ExprParse::NodeVec Nodes;
	ExprParse::Unit*   Unit;
};

class DifferentialParser
{
public:
    DifferentialParser();
    ~DifferentialParser();



};

#endif //DifferentialParser_H
