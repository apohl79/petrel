base64 Library
==============

Encode and decode base64 data.

Synopsis
^^^^^^^^

Usage in LUA::

  encoded = base64.encode("plain text data")

  plain_text = base64.decode(encoded)

.. function:: base64.encode(input)

   Encode input data to base64.
              
   :param string input: The input string to be encoded
   :return: (*string*) - The encoded string

.. function:: base64.decode(input)

   Decode base64 input data.
              
   :param string input: The input string to be decoded
   :return: (*string*) - The decoded string

                        
