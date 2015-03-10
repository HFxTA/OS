#include <algorithm>
#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <cstddef>
#include <cassert>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <cstdlib>

enum Kind
{
	READY,
	SCHED,
	WAIT,
	FINISH
};

class Process
{
public:
	Process(std::string const & id, std::size_t start, std::size_t length)
		: id_(id)
		, start_(start)
		, length_(length)
	{
		assert(length > 0);
		events_[0] = READY;
		events_[length] = FINISH;
		events_[static_cast<std::size_t>(-1)] = FINISH;
	}

	Process()
	{ }

	std::string const & id() const
	{ return id_; }

	std::size_t start() const
	{ return start_; }

	std::size_t length() const
	{ return length_; }

	void add_io(std::size_t start, std::size_t length)
	{
		events_.insert(std::make_pair(start, WAIT));
		events_.insert(std::make_pair(start + length, READY));
	}

	Kind state(std::size_t time) const
	{
		std::map<std::size_t, Kind>::const_iterator const it = events_.lower_bound(time);
		if (it == events_.end())
			return FINISH;
		return it->second;
	}

	std::size_t remain(std::size_t time) const
	{ return events_.lower_bound(time + 1)->first - time; }

private:
	std::string id_;
	std::size_t start_;
	std::size_t length_;
	std::map<std::size_t, Kind> events_;
};

typedef std::multiset<Process> Processes;

bool operator<(Process const &lhs, Process const &rhs)
{ return lhs.id() < rhs.id(); }

std::istream & operator>>(std::istream & in, Process & data)
{
	std::string input;
	if (!std::getline(in, input))
		return in;

	std::stringstream strstr(input);

	std::string id;
	if (!(strstr >> id))
		return in;

	std::size_t start = 0;
	if (!(strstr >> start))
	   return in;

	std::size_t len = 0;
	if (!(strstr >> len))
		return in;

	data = Process(id, start, len);
	std::size_t iostart = 0, iolen = 0;
	while (strstr >> iostart >> iolen)
		data.add_io(iostart, iolen);

	return in;
}

struct Reschedule
{
	std::size_t first;
	std::string second;

	Reschedule(std::size_t time, std::string const &id)
		: first(time)
		, second(id)
	{ }
};

std::ostream & operator<<(std::ostream & out, Reschedule const & resched)
{
	out << resched.first << " " << resched.second << std::endl;
	return out;
}

typedef std::vector<Reschedule> Schedule;

struct QueueEvent
{
	Kind kind;
	Process const * process;
	std::size_t global;
	std::size_t local;

	QueueEvent(Kind st, Process const * proc, std::size_t glob, std::size_t loc)
		: kind(st)
		, process(proc)
		, global(glob)
		, local(loc)
	{ }
};

bool operator<(QueueEvent const & lhs, QueueEvent const & rhs)
{
	if (lhs.global < rhs.global)
		return true;

	if (lhs.global == rhs.global && lhs.kind < rhs.kind)
		return true;

	return false;
}

typedef std::multiset<QueueEvent> EventQueue;

struct ReadyProcess
{
	Process const * process;
	std::size_t local;
	std::size_t remain;

	ReadyProcess(Process const * proc, std::size_t loc)
		: process(proc)
		, local(loc)
		, remain(proc->remain(loc))
	{ }

	ReadyProcess()
	{ }
};

bool operator<(ReadyProcess const & lhs, ReadyProcess const & rhs)
{ return (lhs.remain < rhs.remain); }

typedef std::multiset<ReadyProcess> ReadyQueue;

void schedule(Processes const & proces, Schedule & events, std::size_t quant)
{
	std::size_t active = proces.size();

	EventQueue queue;
	ReadyQueue ready;

	Process const idle("IDLE", 0, static_cast<std::size_t>(-1));
	queue.insert(QueueEvent(SCHED, &idle, 0, 0));

	for (Processes::const_iterator it = proces.begin(); it != proces.end(); ++it)
		queue.insert(QueueEvent(READY, &(*it), it->start(), 0));

	while (active > 0)
	{
		QueueEvent event = *queue.begin(); queue.erase(queue.begin());

		if (event.kind == READY || event.kind == SCHED)
			ready.insert(ReadyProcess(event.process, event.local));

		if (event.kind == WAIT)
		{
			std::size_t const remain = event.process->remain(event.local);
			queue.insert(QueueEvent(READY, event.process,
				event.global + remain, event.local + remain));
		}

		if (event.kind == WAIT || event.kind == SCHED || event.kind == FINISH)
		{
			if (event.kind == FINISH && --active == 0)
				return;

			bool const in_quant = (ready.begin()->remain <= quant);
			std::size_t time = 0;
			std::string name;

			std::cin >> time >> name;

			if (time != event.global)
			{
				std::cout << "rescheduling expected at " << event.global
					<< " but " << time << " received" << std::endl;
				return;
			}

			ReadyQueue app;
			for (ReadyQueue::const_iterator it = ready.begin(); it != ready.end(); ++it)
			{
				bool const this_in_quant = it->remain <= quant;
				if ((in_quant == this_in_quant) && (it->process->id() != "IDLE"))
					app.insert(*it);
			}

			if (name == "IDLE" && !app.empty())
			{
				std::cout << "ERROR: at time " << time << " possible process are: ";
				for (ReadyQueue::const_iterator it = app.begin(); it != app.end(); ++it)
					std::cout << it->process->id() << " ";
				std::cout << std::endl;
				exit(-1);
			}

			if (name != "IDLE" && app.empty())
			{
				std::cout << "ERROR: at time " << time << " no possible processes" << std::endl;
				exit(-1);
			}

			bool found = false;
			for (ReadyQueue::const_iterator it = app.begin(); it != app.end() && !found; ++it)
			{
				if (it->process->id() == name)
					found = true;
			}

			if (!found && (name != "IDLE"))
			{
				std::cout << "ERROR: at time " << time << " possible process are: ";
				for (ReadyQueue::const_iterator it = app.begin(); it != app.end(); ++it)
					std::cout << it->process->id() << " ";
				std::cout << std::endl;
				exit(-1);
			}

			ReadyProcess proc;
			for (ReadyQueue::const_iterator it = ready.begin(); it != ready.end(); ++it)
			{
				if (it->process->id() == name)
				{
					proc = *it;
					ready.erase(it);
					break;
				}
			}

			if (proc.process == &idle && !queue.empty())
				queue.insert(
						QueueEvent(SCHED,
							proc.process,
							event.global + std::min(queue.begin()->global - event.global, quant),
							event.local + std::min(queue.begin()->global - event.global, quant))
						);
			else if (proc.remain > quant)
				queue.insert(
						QueueEvent(SCHED,
							proc.process,
							event.global + quant,
							proc.local + quant)
						);
			else
				queue.insert(
						QueueEvent(proc.process->state(proc.local + proc.remain),
							proc.process,
							event.global + proc.remain,
							proc.local + proc.remain)
						);

			events.push_back(Reschedule(event.global, proc.process->id()));
		}
	}
}

int main(int argc, char **argv)
{
	if (argc < 2)
		return 1;

	Processes proces;
	std::vector<Reschedule> events;

	std::ifstream task(argv[1]);

	std::string line;
	std::getline(task, line);

	std::stringstream input(line);
	std::size_t quant = 0;
	input >> quant;

	while (std::getline(task, line))
	{
		Process proc;
		std::stringstream proc_line(line);
		if (proc_line >> proc)
			proces.insert(proc);
	}

	schedule(proces, events, quant);

	return 0;
}
