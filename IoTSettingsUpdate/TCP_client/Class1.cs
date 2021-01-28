using System;
using System.Threading;

namespace AppBeat.Utility
{
    public static class Concurrency
    {
        /// Method tries to run in thread-safe manner but waits max milliseconds for lock. If it can not acquire lock in specified time, exception is thrown.
        /// This method can be used to prevents deadlock scenarios.
        /// 
        public static void LockWithTimeout(object lockObj, Action doAction, int millisecondsTimeout = 15000)
        {
            bool lockWasTaken = false; var temp = lockObj;
            try
            {
                Monitor.TryEnter(temp, millisecondsTimeout, ref lockWasTaken);
                if (lockWasTaken)
                {
                    doAction();
                }
                else
                {
                    throw new Exception("Could not get lock");
                }
            }
            finally { if (lockWasTaken) { Monitor.Exit(temp); } }
        }
    }
}