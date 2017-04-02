/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2017, Sandeep Prakash
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*******************************************************************************
 * Copyright (c) 2017, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   thread.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Apr 1, 2017
 *
 * \brief  
 *
 ******************************************************************************/

/********************************** INCLUDES **********************************/
#include <iostream>

#include "thread.hpp"

/********************************* CONSTANTS **********************************/

/*********************************** MACROS ***********************************/

/******************************** ENUMERATIONS ********************************/

/************************* STRUCTURE/UNION DATA TYPES *************************/

/************************ STATIC FUNCTION PROTOTYPES **************************/

/****************************** LOCAL FUNCTIONS *******************************/
using namespace std;

void *Thread::threadFunc (void *this_) {
  Thread *t = (Thread *) this_;
  t->run();
  return NULL;
}

void Thread::run () {
  cout << "Entering thread func 1" << endl;
  while (true) {
    if (!mJobQueue->empty ()) {
      cout << "New Job" << endl;
      mMutex.lock ();
      ThreadJob job = mJobQueue->at(0);
      mJobQueue->pop_front();
      mMutex.unlock ();
      runJob (job);

    }
    else {
      cout << "Waiting for job" << endl;
      std::unique_lock<std::mutex> lk (mMutex);
      mCondition->wait(lk);
    }
  }
}

void Thread::runJob (ThreadJob &job) {
  cout << "Running new job" << endl;
  job.routine (job.arg);
}

void Thread::addJob (ThreadJob &job) {
  cout << "Adding Job" << endl;
  std::lock_guard<std::mutex> lock(mMutex);
  mJobQueue->push_back(job);
  mCondition->notify_one();
}

Thread::Thread () :
  mMutex () {
  mCondition = new std::condition_variable ();
  mJobQueue = new std::deque <ThreadJob> ();
  pthread_create(&mThread, NULL, Thread::threadFunc, this);
}

Thread::~Thread () {
}
