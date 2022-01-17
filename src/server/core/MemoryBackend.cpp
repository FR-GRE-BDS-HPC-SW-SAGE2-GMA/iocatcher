/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

/****************************************************/
//internal
#include "MemoryBackend.hpp"

/****************************************************/
using namespace IOC;

/****************************************************/
/**
 * Basic constructor of the memory backend.
 * @param lfDomain Define the libfabric domain to be used for memory
 * registration after allocating new memory.
**/
MemoryBackend::MemoryBackend(LibfabricDomain * lfDomain)
{
	this->lfDomain = lfDomain;
}

/****************************************************/
/**
 * Virtual destructor for inheritance. Currently do nothing.
**/
MemoryBackend::~MemoryBackend(void)
{
	//nothing to do
}

/****************************************************/
/**
 * Return the libfabric domain in use.
**/
LibfabricDomain * MemoryBackend::getLfDomain(void)
{
	return this->lfDomain;
}
