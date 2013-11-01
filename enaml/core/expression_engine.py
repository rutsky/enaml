#------------------------------------------------------------------------------
# Copyright (c) 2013, Nucleic Development Team.
#
# Distributed under the terms of the Modified BSD License.
#
# The full license is in the file COPYING.txt, distributed with this software.
#------------------------------------------------------------------------------
from atom.api import Atom, List, Typed
from atom.datastructures.api import sortedmap


class ReadHandler(Atom):
    """ A base class for defining expression read handlers.

    """
    def __call__(self, owner, name):
        """ Evaluate and return the expression value.

        This method must be implemented by subclasses.

        Parameters
        ----------
        owner : Declarative
            The declarative object on which the expression should
            execute.

        name : str
            The attribute name on the declarative object for which
            the expression is providing a value.

        Returns
        -------
        result : object
            The evaluated value of the expression.

        """
        raise NotImplementedError


class WriteHandler(Atom):
    """ A base class for defining expression write handlers.

    """
    def __call__(self, owner, name, change):
        """ Write the change to the expression.

        This method must be implemented by subclasses.

        Parameters
        ----------
        owner : Declarative
            The declarative object on which the expression should
            execute.

        name : str
            The attribute name on the declarative object for which
            the expression is providing a value.

        change : dict
            The change dict generated by the declarative object.

        """
        raise NotImplementedError


class HandlerPair(Atom):
    """ An object which represents a pair of handlers.

    A handler pair is used to provide read and write handlers to the
    expression engine as the result of an operator call. The runtime
    adds the pair returned by the operator to the relevant expression
    engine.

    """
    #: The read handler for the pair. This may be None if the given
    #: operator does not support read semantics.
    reader = Typed(ReadHandler)

    #: The write handler for the pair. This may be None if the given
    #: operator does not support write semantics.
    writer = Typed(WriteHandler)


class HandlerSet(Atom):
    """ A class which aggregates handler pairs for an expression.

    This class is used internally by the runtime to manage handler
    pair precedence. It should not be used directly by user code.

    """
    #: The handler pair which holds the precedent read handler.
    read_pair = Typed(HandlerPair)

    #: The list of handler pairs which hold write handlers. The pairs
    #: are ordered from oldest to newest (execution order).
    write_pairs = List(HandlerPair)

    #: The complete list of handler pairs for an attribute.
    all_pairs = List(HandlerPair)

    def copy(self):
        """ Create a copy of this handler set.

        Returns
        -------
        result : HandlerSet
            A copy of the handler set with independent lists.

        """
        new = HandlerSet()
        new.read_pair = self.read_pair
        new.write_pairs = self.write_pairs
        new.all_pairs = self.all_pairs
        return new


class ExpressionEngine(Atom):
    """ A class which manages reading and writing bound expressions.

    """
    #: A private mapping of string attribute name to HandlerSet.
    _handlers = Typed(sortedmap, ())

    #: A private set of guard tuples for preventing feedback loops.
    _guards = Typed(set, ())

    def __nonzero__(self):
        """ Get the boolean value for the engine.

        An expression engine will test boolean False if there are
        no handlers associated with the engine.

        """
        return len(self._handlers) > 0

    def add_pair(self, name, pair):
        """ Add a HandlerPair to the engine.

        Parameters
        ----------
        name : str
            The name of the attribute to which the pair is bound.

        pair : HandlerPair
            The pair to bind to the expression.

        """
        handler = self._handlers.get(name)
        if handler is None:
            handler = self._handlers[name] = HandlerSet()
        handler.all_pairs.append(pair)
        if pair.reader is not None:
            handler.read_pair = pair
        if pair.writer is not None:
            handler.write_pairs.append(pair)

    def read(self, owner, name):
        """ Compute and return the value of an expression.

        Parameters
        ----------
        owner : Declarative
            The declarative object which owns the engine.

        name : str
            The name of the relevant bound expression.

        Returns
        -------
        result : object or NotImplemented
            The evaluated value of the expression, or NotImplemented
            if there is no readable expression in the engine.

        """
        handler = self._handlers.get(name)
        if handler is not None:
            pair = handler.read_pair
            if pair is not None:
                return pair.reader(owner, name)
        return NotImplemented

    def write(self, owner, name, change):
        """ Write a change to an expression.

        This method will not run the handler if its paired read handler
        is actively updating the owner attribute. This behavior protects
        against feedback loops and saves useless computation.

        Parameters
        ----------
        owner : Declarative
            The declarative object which owns the engine.

        name : str
            The name of the relevant bound expression.

        change : dict
            The change dictionary generated by the declarative object
            which owns the engine.

        """
        handler = self._handlers.get(name)
        if handler is not None:
            guards = self._guards
            for pair in handler.write_pairs:
                key = (owner, pair)
                if key not in guards:
                    guards.add(key)
                    try:
                        pair.writer(owner, name, change)
                    finally:
                        guards.remove(key)

    def update(self, owner, name):
        """ Update the named attribute of the owner.

        This method will update the named attribute by re-reading the
        expression and setting the value of the attribute. This method
        will not run the handler if its paired write handler is actively
        updating the owner attribute. This behavior protects against
        feedback loops and saves useless computation.

        Parameters
        ----------
        owner : Declarative
            The declarative object which owns the engine.

        name : str
            The name of the relevant bound expression.

        """
        handler = self._handlers.get(name)
        if handler is not None:
            pair = handler.read_pair
            if pair is not None:
                guards = self._guards
                key = (owner, pair)
                if key not in guards:
                    guards.add(key)
                    try:
                        setattr(owner, name, pair.reader(owner, name))
                    finally:
                        guards.remove(key)

    def copy(self):
        """ Create a copy of the expression engine.

        Returns
        -------
        result : ExpressionEngine
            A copy of the engine with independent handler sets.

        """
        new = ExpressionEngine()
        handlers = sortedmap()
        for key, value in self._handlers.items():
            handlers[key] = value.copy()
        new._handlers = handlers
        return new