import threading
from contextlib import contextmanager

class ReadWriteLock(object):
    ''' This reader writer lock gurantees write order, but not
    read order and is generally biased towards allowing writes
    if they are available to prevent starvation.

    TODO:

    * allow user to choose between read/write/random biasing
    - currently write biased
    - read biased allow N readers in queue
    - random is 50/50 choice of next
    '''

    def __init__(self):
        ''' Initializes a new instance of the ReadWriteLock
        '''
        self.queue   = []                                  # the current writer queue
        self.lock    = threading.Lock()                    # the underlying condition lock
        self.read_condition = threading.Condition(self.lock) # the single reader condition
        self.readers = 0                                   # the number of current readers
        self.writer  = False                               # is there a current writer

    def __is_pending_writer(self):
        return (self.writer                                # if there is a current writer
            or (self.queue                                 # or if there is a waiting writer
           and (self.queue[0] != self.read_condition)))    # or if the queue head is not a reader

    def acquire_reader(self):
        ''' Notifies the lock that a new reader is requesting
        the underlying resource.
        '''
        with self.lock:
            if self.__is_pending_writer():                 # if there are existing writers waiting
                if self.read_condition not in self.queue:  # do not pollute the queue with readers
                    self.queue.append(self.read_condition) # add the readers in line for the queue
                while self.__is_pending_writer():          # until the current writer is finished
                    self.read_condition.wait(1)            # wait on our condition
                if self.queue and self.read_condition == self.queue[0]: # if the read condition is at the queue head
                    self.queue.pop(0)                      # then go ahead and remove it
            self.readers += 1                              # update the current number of readers

    def acquire_writer(self):
        ''' Notifies the lock that a new writer is requesting
        the underlying resource.
        '''
        with self.lock:
            if self.writer or self.readers:                # if we need to wait on a writer or readers
                condition = threading.Condition(self.lock) # create a condition just for this writer
                self.queue.append(condition)               # and put it on the waiting queue
                while self.writer or self.readers:         # until the write lock is free
                    condition.wait(1)                      # wait on our condition
                self.queue.pop(0)                          # remove our condition after our condition is met
            self.writer = True                             # stop other writers from operating

    def release_reader(self):
        ''' Notifies the lock that an existing reader is
        finished with the underlying resource.
        '''
        with self.lock:
            self.readers = max(0, self.readers - 1)        # readers should never go below 0
            if not self.readers and self.queue:            # if there are no active readers
                self.queue[0].notify_all()                 # then notify any waiting writers

    def release_writer(self):
        ''' Notifies the lock that an existing writer is
        finished with the underlying resource.
        '''
        with self.lock:
            self.writer = False                            # give up current writing handle
            if self.queue:                                 # if someone is waiting in the queue
                self.queue[0].notify_all()                 # wake them up first
            else: self.read_condition.notify_all()         # otherwise wake up all possible readers

    @contextmanager
    def get_reader_lock(self):
        ''' Wrap some code with a reader lock using the
        python context manager protocol::

            with rwlock.get_reader_lock():
                do_read_operation()
        '''
        try:
            self.acquire_reader()
            yield self
        finally: self.release_reader()

    @contextmanager
    def get_writer_lock(self):
        ''' Wrap some code with a writer lock using the
        python context manager protocol::

            with rwlock.get_writer_lock():
                do_read_operation()
        '''
        try:
            self.acquire_writer()
            yield self
        finally: self.release_writer()
