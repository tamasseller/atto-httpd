Minimal HTTP daemon template
============================

This is a rudimentary HTTP server, based on the node.js version of the ngnix HTTP parser,
and also a generous amount genuine code. 
It is intended to be used mainly in microcontroller environment.

This library uses zero-overhead modern C++ features, and that means that 
**it requires a decent compiler with stable C++11 support** (like GCC 5+).

Features
--------

 - Very small, ~350 bytes per client memory footprint used only via static allocation (_no malloc_) 
 - Efficient, _zero-copy parsing_ of input.
 - Content can be sent and received with zero-copy semantics.
 - No hard-coded dependency on _network or file access_.
 - Auth digest support (simplest, RFC2069 version).
 - Supports WebDAV (partial level 1 compliance, no locks) -> can be mounted on PC. 
 - Zero overhead integration with CRTP based dependency injection.
 
 
Limitations
-----------
 
 - Can not parse arbitrarily long dav requests, due to memory limitation.
 - Path element length is limited to 32 bytes.
 - Single user-realm-password triplet.
 - No support for Etags, preconditions and Range queries.
 - Webdav xml is completely unvalidated (even close tags are not checked to be matching).
 
How to use
----------
 
_Check out the test/end2end directory for an example of usage._

The user is required to subclass the HttpLogic template in a CRTP fashion,
passing the the derived type as the first type argument to the parent. 
Also some static configuration is required to be passed to it, as the second argument.
See the example for details.

As the http server logic is provided in the form of a class template, the library mainly
consists of header files.
Only the 'third-party' parts contain proper compilation units: 
namely the nginx parser has the **http-parser/http_parser.c** and the md5 implementation has **md5/md5.c** in it.
Because of the small number of these files, there is no build utility provided for a regular library build.
You can **add these directly to your own projects build system**.