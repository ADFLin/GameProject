#pragma once
#ifndef SGNode_H_C94AE3CE_1A99_46F8_9136_B453A1D9DE6A
#define SGNode_H_C94AE3CE_1A99_46F8_9136_B453A1D9DE6A


class SGNodeBase
{
public:



};

class SGEntryNode : public SGNodeBase
{




};

class SGStateNode : public SGNodeBase
{
public:
	void active();
	void deactive();

};


class SGActionNode : public SGNodeBase
{
public:
	void execute();

};
#endif // SGNode_H_C94AE3CE_1A99_46F8_9136_B453A1D9DE6A
