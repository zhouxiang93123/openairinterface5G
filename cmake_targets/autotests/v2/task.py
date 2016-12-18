import os, time, threading, sys, os, errno, select
from connection import connection

import utils

class ReaderThread(threading.Thread):
    def __init__(self, fdin, logfile):
        threading.Thread.__init__(self)
        self.fdin = fdin
        self.logfile = logfile

    def run(self):
        try:
            outfile = open(self.logfile, "w")
        except BaseException, e:
            print "ERROR: ReaderThread: " + self.logfile
            print e
            os._exit(1)
        while True:
            try:
               (a, b, c) = select.select([ self.fdin ], [], [ self.fdin ])
            except BaseException, e:
               print "ERROR: ReaderThread: select failed"
               print e
               os._exit(1)
            try:
                z = os.read(self.fdin, 1024)
            except OSError, e:
                if e.errno == errno.EIO:
                  #pipe has died, quit the thread
                  break
                else:
                  print "ERROR: ReaderThread: unhandled error"
                  print e
            except BaseException, e:
                print "ERROR: ReaderThread: unhandled error"
                print e
                break
            try:
                outfile.write(z)
                outfile.flush()
            except BaseException, e:
                print "ERROR: ReaderThread: " + self.logfile
                print e
                os._exit(1)
        try:
            outfile.close()
        except BaseException, e:
            print "ERROR: ReaderThread: " + self.logfile
            print e
            os._exit(1)
        #close the pipe, don't care about errors
        try:
            os.close(self.fdin)
        except:
            pass

class Task:
    def __init__(self, action, description, machine, user, password, env,
                 logfile, post_action = None):
        self.action      = action
        self.description = description
        self.machine     = machine
        self.user        = user
        self.password    = password
        self.env         = env
        self.post_action = post_action

        self.connection = connection(description, machine, user, password)
        self.reader = ReaderThread(self.connection.fd, logfile)
        self.reader.start()

        self.connection.send('export PS1=\n')
        self.connection.send('set +o emacs\n')
        self.connection.send('echo\n')
        self.connection.send('echo\n')
        self.connection.send("echo -e '" + utils.GREEN +
                             '---------------------------------------------'
                             + utils.RESET + "'\n")
        self.connection.send('echo\n')
        self.connection.send("echo -n -e '" + utils.YELLOW +
                             "COMMANDS START: " +
                             utils.RESET + "'\n")
        self.connection.send('date\n')

        for l in env:
            self.connection.send('export ' + l + '\n')

        try:
            with open(action) as f:
                for line in f:
                    self.connection.send("echo -n -e '" + utils.GREEN +
                                         "RUNNING: " + utils.RESET + "'\n")
                    self.connection.send("echo '" +
                                         line.replace('\n','')
                                             .replace("'", "'\\''") + "'\n")
                    self.connection.send(line)
                    self.connection.send("if [ $? != 0 ]; then " +
                                         "echo -e '" + utils.RED +
                                         "TEST_SETUP_ERROR: " + utils.RESET +
                                         "last command failed, exiting'; " +
                                         "date; exit 1; fi\n")
        except BaseException, e:
            print "ERROR: task failed:    " + str(e)
            print "ERROR: action is:      " + action
            print "ERROR: description is: " + description
            os._exit(1)

        self.connection.send("echo -n -e '" + utils.YELLOW +
                             "COMMANDS DONE: " +
                             utils.RESET + "'\n")
        self.connection.send('date\n')
        self.connection.send("echo -e '" + utils.GREEN +
                             "TEST_SETUP_SUCCESS" + utils.RESET + "'\n")
        self.connection.send('exit\n')

    def wait(self, timeout=-1):
        try:
            (pid, ret) = os.waitpid(self.connection.pid, 0)
        except KeyboardInterrupt, e:
            print "ERROR: ctrl+c catched!" + str(e)
            os._exit(1)
        except BaseException, e:
            print "ERROR: " + str(e)
            os._exit(1)
        return ret

    def postaction(self):
        if self.post_action != None:
            out = utils.quickshell(self.post_action)
            if len(out):
                print "INFO: task '" + self.description + \
                      "' post_action '" + self.post_action + "' says: "
                for l in out.splitlines():
                    print "INFO:     " + l
