/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2022 Sebastien Valat                   *
*****************************************************/
#ifndef IOC_TASK_HPP
#define IOC_TASK_HPP

/****************************************************/
#include <cassert>
#include "base/common/Debug.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
enum TaskStage
{
	STAGE_PREPARE,
	STAGE_ACTION,
	STAGE_POST,
	STAGE_FINISHED
};

/****************************************************/
/**
 * Define what is a task to be deferred to a worker thread. A task has two
 * actions. A run action to be called in the worker thread and a post-action
 * to be called on the main network handling thread when the task has been
 * performed.
**/
class Task
{
	public:
		inline Task(void);
		virtual inline ~Task(void);
		inline bool runNextStage(void);
		inline bool runNextStage(TaskStage expectedStage);
		inline void performAll(void);
		inline bool isImmediate(void) const;
		inline void markAsImmediate(void);
		inline TaskStage getStage(void) const;
	protected:
		/** Prepage stage to compute the buffer addresses for the run stage. */
		virtual void runPrepare(void) = 0;
		/** Perform the given action in the worker thread. */
		virtual void runAction(void) = 0;
		/** 
		 * To be called in the network handling thread after the action has 
		 * been performed by the worker. 
		**/
		virtual void runPostAction(void) = 0;
	private:
		/** Define if the task need to be runned immediately of not. **/
		bool immediate;
		/** Next stage to perform. **/
		TaskStage nextStage;
};

/****************************************************/
/** Default constructor. **/
Task::Task(void)
{
	this->immediate = false;
	this->nextStage = STAGE_PREPARE;
}

/****************************************************/
/**
 * Destructor, only checks that we ran everything.
**/
Task::~Task(void)
{
	assert(this->nextStage == STAGE_FINISHED || this->nextStage == STAGE_PREPARE);
};

/****************************************************/
/** Run next stage. **/
bool Task::runNextStage(void)
{
	//var
	bool res = false;

	//apply
	switch(this->nextStage) {
		case STAGE_PREPARE:
			this->runPrepare();
			this->nextStage = STAGE_ACTION;
			break;
		case STAGE_ACTION:
			this->runAction();
			this->nextStage = STAGE_POST;
			break;
		case STAGE_POST:
			this->runPostAction();
			this->nextStage = STAGE_FINISHED;
			res = true;
			break;
		default:
			IOC_FATAL("Invalid stage to run !");
			break;
	}

	//ret
	return res;
}

/****************************************************/
/** Run next stage and check we are at the expected one. **/
bool Task::runNextStage(TaskStage expectedStage)
{
	//check
	assumeArg(this->nextStage == expectedStage, "Invalid stage (%1), expected (%2)")
		.arg(this->nextStage)
		.arg(expectedStage)
		.end();

	//run
	return this->runNextStage();
}

/****************************************************/
/** Perform both actions in one go. **/
void Task::performAll(void)
{
	while(this->runNextStage()) {};
}

/****************************************************/
/** 
 * Tell to the worker manager to run the task immediatly and not send it to a 
 * worker thread.
 * This is usefull to make the small task optimization not entering in
 * thread queue communications and get a fast local execution.
**/
bool Task::isImmediate(void) const
{
	return this->immediate;
};

/****************************************************/
/** 
 * Tell to the worker manager to run the task immediatly and not send it to a 
 * worker thread.
 * This is usefull to make the small task optimization not entering in
 * thread queue communications and get a fast local execution.
**/
void Task::markAsImmediate(void)
{
	this->immediate = true;
};

/****************************************************/
/** Return the current stage of the task. **/
TaskStage Task::getStage(void) const
{
	return this->nextStage;
};

}

#endif //IOC_TASK_HPP
