import threading, os

class MachineWaiterThread(threading.Thread):
    def __init__(self, machine, tasks):
        threading.Thread.__init__(self)
        self.machine = machine
        self.tasks = tasks

    def run(self):
        try:
            for task in self.tasks:
                ret = task.wait()
                if ret != 0:
                    print "ERROR: task '" + task.description + "' failed " + \
                          "on machine " + self.machine.name
                task.postaction()
            self.machine.unbusy()
        except BaseException, e:
            print "ERROR: MachineWaiterThread: " + str(e)
            os._exit(1)

class Machine():
    def __init__(self, machine, cond):
        self.name = machine
        self.free = True
        self.cond = cond
    def busy(self, tasks):
        waiter = MachineWaiterThread(self, tasks)
        waiter.start()
    def unbusy(self):
        self.cond.acquire()
        self.free = True
        self.cond.notify()
        self.cond.release()

class MachineList():
    def __init__(self, list):
        self.list = []
        self.cond = threading.Condition()
        for m in list:
            self.list.append(Machine(m, self.cond))

    def get_free_machine(self):
        try:
            self.cond.acquire()
            while True:
                free_machine = None
                for m in self.list:
                    if m.free == True:
                        free_machine = m
                        break
                if free_machine != None:
                    break
                self.cond.wait()
            free_machine.free = False
            self.cond.release()
        except BaseException, e:
            print "ERROR: machine_list: " + str(e)
            os._exit(1)
        return free_machine
