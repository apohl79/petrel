Docker Guide
============

`Docker <https://www.docker.com/>`_ is a container manager for Linux that became quite popular. We have created an `image <https://hub.docker.com/r/apohl79/petrel/>`_ and pushed it to Docker Hub (Dockers public image registry).

You can run a container veray quickly via::

  $ docker run --name petrel-test -p 8585:8585 -d apohl79/petrel

Now you can fire a test request to petrel. If you are on linux you can just use localhost. On OS X for example you have to use the IP of your docker VM (should be ``192.168.99.100`` in a default installation)::

  $ curl "http://localhost:8585/"
  <html><head><title>petrel test page</title></head><body><h1>petrel test page</h1>We got called 6 times</body></html>

Use your own code
-----------------

You have two ways of using this image with your own service code:

#. Use volumes to mount you service code into the docker container::

     $ docker run --name myservice -v /your/lua/dir:/lua:ro -p 8585:8585 -d apohl79/petrel

   The same way cou change petrels configuration file.

#. Create your own image:

   #. Create a new directory and enter it
   #. Create a Dockerfile with the following content::

        FROM apohl79/petrel:latest
        COPY /your/lua/dir/ /lua/

      Again, if you have a different configuration, just copy it into your image to ``/etc/petrel/petrel.conf`` by using another ``COPY`` command.

   #. Now build your image::

        $ docker build -t myservice .

   #. And run it::

        $ docker run --name myservice -p 8585:8585 -d myservice
