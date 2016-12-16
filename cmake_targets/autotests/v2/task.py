import os
from ssh import connection

class Task:
    def __init__(self, action, description, machine, user, password, env):
        self.action      = action
        self.description = description
        self.machine     = machine
        self.user        = user
        self.password    = password
        self.env         = env

        print description
        print machine
        print user
        print password

        print 'create connection'
        self.connection = connection(description, machine, user, password)
        print 'done'

        print self.connection.pid
        print self.connection.fd

#        print "let's read"
#        try:
#            z = os.read(self.connection.fd, 1024)
#        except Exception, e:
#            print "nothing returned"
#            print e
#            z = ""
#        print z

        self.connection.send('date\n')

        for l in env:
            self.connection.send('export ')
            self.connection.send(l)
            self.connection.send('\n')

        try:
            with open(action) as f:
                for line in f:
                    print "LINE:" + line
                    self.connection.send(line)
        except Exception, e:
            print "ERROR: task failed:    " + str(e)
            print "ERROR: action is:      " + action
            print "ERROR: description is: " + description
            exit(1)

        self.connection.send('date\n')
        self.connection.send('exit\n')

    def wait(self, timeout=-1):
        (pid, ret) = os.waitpid(self.connection.pid, 0)
        return ret

def new_task(action, description, machine, user, password, env):
    return Task(action, description, machine, user, password, env)
