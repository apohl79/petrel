Configuration Options
=====================

All configuration options can be specified via commandline or configuration file. Parameters are grouped together. A paramter on the commandline has the form --<group>.<parameter>.

The following commandline options are available:

=========================================  ===========
Option                                     Description
=========================================  ===========
**Server options:**                        Group name: *server*
http2                                      Run a HTTP/2 server. Default is HTTP/1.
workers                                    Number of worker threads. By setting this to 0, 1 worker per
\                                          CPU gets created. Default is 1.
listen arg                                 Listen on the given IP/name.
port arg (=8585)                           Server port for TCP
backlog arg (=0)                           Listen backlog. Set this to 0 for max.
http2.tls                                  Enable SSL/TLS support for HTTP2.
http2.key-file arg                         SSL/TLS private key file
http2.crt-file arg                         SSL/TLS certificate file
dns-cache-ttl arg (=5)                     DNS cache TTL in minutes
\
**LUA options:**                           Group name: *lua*
root arg                                   The lua script root. All .lua files will be loaded.
statebuffer arg (=500)                     The lua state buffer controlls how many lua state objects will be
\                                          kept available by the lua engine for request handling to avoid
\                                          creating states at handling time.
lua.devmode                                Activate the lua devmode. In this mode the server will reload the
\                                          lua scripts on every single request.
\
**Logging options:**                       Group name: *log*
syslog                                     Log to syslog.
level arg (=6)                             Log level: 0=emerg, 1=alert, 2=crit, 3=err, 4=warn, 5=notice,
\                                          6=info, 7=debug
\
**Metrics options:**                       Group name: *metrics*
log arg (=0)                               Enable metrics logging every N seconds.
graphite                                   Enable sending metrics to graphite.
graphite.interval arg (=10)                Send the metrics every N seconds.
graphite.host arg                          The graphite host
graphite.port arg (=2003)                  The graphite port
graphite.prefix arg (=petrel)              The name prefix for metrics send to graphite. A metric name will
\                                          be constructed as follows: <prefix>.<hostname>.<metricname>
=========================================  ===========

You can use group sections in a configuration file. For example::

  [server]
  listen=localhost
  port=8585
  http2=1

  [log]
  syslog=1

The above is equivalent to passing *"--server.listen=localhost --server.port=8585 --server.http2 --log.syslog"* via commandline.
