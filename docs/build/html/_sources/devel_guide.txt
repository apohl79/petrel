Developer Guide
===============

This guide is for those who want contribute code to petrel. It describes how to setup a development environment, some conventions and instructions to submit patches.

Development Environment
-----------------------

The :doc:`installation_guide` describes the general procedure of building petrel and it's dependencies. Here we show only how to get `valgrind <http://valgrind.org/>`_ working, as valgrind has problems with stack switching.

#. **Boost**

   To enable valgrind support in the context and fiber libraries, recompile boost with the ``valgrind=on`` option::
     
     $ sudo ./b2 -q --user-config=user-config.jam cxxflags="-std=c++11 -fPIC" threading=multi link=static valgrind=on install


#. **Petrel**

   For petrel you have to enable valgrind support by passing ``-DVALGRIND=on`` to cmake::
  
     $ cmake -DBOOST_ROOT=/usr/local/boost_head -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -DVALGRIND=on ..


Contributions
-------------

We accept patches via github pull requests. A brief description of how to get started can be found below.

First Steps
^^^^^^^^^^^

* Fork the petrel repo on github
* Follow the guide to setup your dev environment
* Write code

Workflow
^^^^^^^^

The workflow to submit a patch would usually look like this:

#. Create a topic branch from master or develop
#. Commit your changes to the topic brnach
#. Add unit tests for new code and amke sure all tests pass
#. Push the changes to the topic branch in your forked repo on github
#. Submit a pull request to ``apohl79/petrel``

The pull request will trigger a build on travis which has to succeed before we will review your patch.

Thanks a lot for your contributions!


Code Formating
^^^^^^^^^^^^^^

We use clang-format. The configuration file ``.clang-format`` is located the the root directory of the repository. You can integrate it into your editor or you setup a pre-commit hook in git (see `here <https://github.com/tatsuhiro-t/nghttp2/blob/master/pre-commit>`_ for an example script).

