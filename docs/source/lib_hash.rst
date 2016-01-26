hash Library
============

Builtin hash functions.

Synopsis
^^^^^^^^

Usage in LUA::

  md5_hash = hash.md5("plain text data")

  sha1_hash = hash.sha1("plain text data")

  fnv_hash = hash.fnv("plain text data")


.. function:: hash.md5(input)

   Hash input data via the SHA1 hash algorithm.
              
   :param string input: The input string to be hashed
   :return: (*string*) - The hash string

.. function:: hash.sha1(input)

   Hash input data via the MD5 hash algorithm.
              
   :param string input: The input string to be hashed
   :return: (*string*) - The hash string

.. function:: hash.fnv(input)

   Hash input data via the FNV hash algorithm.
              
   :param string input: The input string to be hashed
   :return: (*integer*) - The hash integer value
