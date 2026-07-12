#pragma once

// Core / generic (ungrouped) actions extracted from ProcessAction().
// These are not tied to any single optional transport feature.

bool tryHandleCommandAction(commandTokens& cmd, String& tmpAction)
{
	//command=****
	if (cmd.command == ACTION_COMMAND)
	{
		String tmpCmd = tmpAction.substring(sizeof(ACTION_COMMAND));
		processCommand(tmpCmd, CHANNEL_UART, true);
	}
	else
	{
		return false;
	}
	return true;
}

bool tryHandleOutputAction(commandTokens& cmd, String& tmpAction)
{
	//set_output?=x
	if (cmd.command == ACTION_SET_OUTPUT)
	{
		set_output(cmd.index, cmd.arguments[0]);
	}
	else
	{
		return false;
	}
	return true;
}

bool tryHandleCounterAction(commandTokens& cmd, String& tmpAction)
{
	//reset_counter? - deprecated
	if (cmd.command == ACTION_RESET_COUNTER)
	{
		if (cmd.index >= 0 && cmd.index < PIN_NUMBER)
		{
			InterruptCounter[cmd.index - 1] = 0;
		}
	}
	//set_counter?=x
	else if (cmd.command == ACTION_SET_COUNTER)
	{
		if (cmd.index >= 0 && cmd.index < PIN_NUMBER)
		{
			InterruptCounter[cmd.index - 1] = cmd.arguments[0].toInt();
		}
	}
	else
	{
		return false;
	}
	return true;
}

#ifdef EVENTS_ENABLE
bool tryHandleEventFlagAction(commandTokens& cmd, String& tmpAction)
{
	//enable_event?
	if (cmd.command == ACTION_ENABLE_EVENT)
	{
		if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
		{
			eventDisabled[cmd.index] = false;
		}
	}
	//disable_event?
	else if (cmd.command == ACTION_DISABLE_EVENT)
	{
		if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
		{
			eventDisabled[cmd.index] = true;
		}
	}
	//set_event_flag?=0/1/off/on
	else if (cmd.command == ACTION_SET_EVENT_FLAG)
	{
		if (cmd.index >= 0 && cmd.index < EVENTS_NUMBER)
		{
			if (cmd.arguments[0] == SWITCH_OFF_NUMBER || cmd.arguments[0] == SWITCH_OFF)
			{
				writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_OFF_NUMBER);
				eventDisabled[cmd.index] = false;
			}
			else if (cmd.arguments[0] == SWITCH_ON_NUMBER || cmd.arguments[0] == SWITCH_ON)
			{
				writeConfigString(EVENTS_ENABLE_addr, EVENTS_ENABLE_size, SWITCH_ON_NUMBER);
				eventDisabled[cmd.index] = true;
			}
		}
	}
	else
	{
		return false;
	}
	return true;
}
#endif

#ifdef SCHEDULER_ENABLE
bool tryHandleClearScheduleAction(commandTokens& cmd, String& tmpAction)
{
	//clear_schedule_exec_time?
	if (cmd.command == ACTION_CLEAR_SCHEDULE_EXEC_TIME)
	{
		if (cmd.index >= 0 && cmd.index < SCHEDULES_NUMBER)
		{
			writeScheduleExecTime(cmd.index, 0);
		}
	}
	else
	{
		return false;
	}
	return true;
}
#endif

bool tryHandleCoreAction(commandTokens& cmd, String& tmpAction)
{
	if (tryHandleCommandAction(cmd, tmpAction)) {}
	else if (tryHandleOutputAction(cmd, tmpAction)) {}
	else if (tryHandleCounterAction(cmd, tmpAction)) {}
#ifdef EVENTS_ENABLE
	else if (tryHandleEventFlagAction(cmd, tmpAction)) {}
#endif
#ifdef SCHEDULER_ENABLE
	else if (tryHandleClearScheduleAction(cmd, tmpAction)) {}
#endif
	else
	{
		return false;
	}
	return true;
}
