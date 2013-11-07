mjson - Modified JSON library
====

Syntax changes
----

Changes are similar to described in following blog post  http://bitsquid.blogspot.com/2009/09/json-configuration-data.html with some additions.

In summary:

 - no {} needed around the whole file;
 - "=" is allowed instead of ":";
 - quotes around the key are optional if key is identifier;
 - commas after values are optional;
 - c-style comments are allowed.

Design
----

Input text is transformed into binary blob. Blob storage is based on ideas from BJSON specification: http://bjson.org, except everything 4 bytes aligned. Strings data storage size is rounded to next multiplier of 4.

Some code ideas are borrowed from another JSON parser: https://github.com/megous/sjson:

 - immediate traversal of binary data instead of pointer following in API implementation
 - use of re2c

Also library design makes possible interchangeably use binary and text representation.
 
License
----

Copyright (c) 2013, Mykhailo Parfeniuk  
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
