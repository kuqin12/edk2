/*
 * MpPipeInternals.h
 *
 *  Created on: Mar 27, 2018
 *      Author: mrabeda
 */

#ifndef MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPPIPEINTERNALS_H_
#define MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPPIPEINTERNALS_H_

#include "MpCommon.h"

VOID
ThreadQueueInit (
  THREAD_QUEUE    *ThreadQueue
  );

VOID
ThreadQueueSleep (
  THREAD_QUEUE    *ThreadQueue
  );

VOID
ThreadQueueWake (
  THREAD_QUEUE    *ThreadQueue
  );

VOID
ThreadQueueWakeAll (
  THREAD_QUEUE    *ThreadQueue
  );

VOID
ConditionInit (
  CONDITION *Condition
  );

VOID
ConditionSignal (
  CONDITION *Condition
  );

VOID
ConditionBroadcast (
  CONDITION *Condition
  );

VOID
ConditionWait (
  CONDITION *Condition,
  SPIN_LOCK *Lock
  );

//
// TODO: ThreadFQueueDestroy
//

#endif /* MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPPIPEINTERNALS_H_ */
