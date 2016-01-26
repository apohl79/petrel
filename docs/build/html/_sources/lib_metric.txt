metric Library
==============

The metric library provides access to petrel's builtin metrics.

Synopsis
^^^^^^^^

Usage in LUA::

  counter = metric()
  counter:register_counter("mycounter")
  counter:increment()
  counter:decrement()
  print(LOG_DEBUG, counter:total())

  meter = metric()
  meter:register_meter("mymeter")
  meter:increment()  
  
  timer = metric()
  timer:register_timer("mytimer")
  timer:update(1.25)
  timer:start_duration()
  <some code you want to measure>
  timer:update_duration()
  <more code you want to measure>
  timer:finish_duration()

.. method:: metric()

   Constructor. After creating a metric object it has to be associated with a metric by calling one of the register_* or get_* methods.

Counter
^^^^^^^

This metric is a simple counter that can be incremented or decremented.

.. method:: register_counter(name)

   Register (create) a new counter metric or load an existing one.
   
   :param string name: The name of the metric to register or load.

.. method:: get_counter(name)

   Load an existing metric.
   
   :param string name: The name of the metric to register or load.

   .. method:: increment([value = 1])

      Increment the counter by the given value.
   
      :param integer value: The value to increment. The default is 1.

   .. method:: decrement([value = 1])

      Decrement the counter by the given value.
   
      :param integer value: The value to decrement. The default is 1.

   .. method:: total()

      :return: (*integer*) - The total value of the counter.

   .. method:: reset()

      Reset the counter to 0.


Meter
^^^^^

A meter can be used to measure rates very efficiently. Petrel uses it to measure the requests per seconds for example. It supports rates over 1min, 5min, 15min and 1hr intervals by implementing EWMA (see: `<https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average>`).

.. method:: register_meter(name)

   Register (create) a new meter metric or load an existing one. 
              
   :param string name: The name of the metric to register or load.

.. method:: get_meter(name)

   Load an existing metric.
   
   :param string name: The name of the metric to register or load.

   .. method:: increment([value = 1])
      :noindex:

      Update the meter with the given value.
   
      :param integer value: The value to increment. The default is 1.

   .. method:: total()
      :noindex:

      :return: (*integer*) - The total value of the meter.


Timer
^^^^^

A timer can be used to perform time measurements. It uses histograms internally to calculate the distribution of the measured times.

.. method:: register_timer(name)

   Register (create) a new counter metric or load an existing one.
              
   :param string name: The name of the metric to register or load.

.. method:: get_timer(name)

   Load an existing metric.
   
   :param string name: The name of the metric to register or load.

   .. method:: update(value)

      Update the timer with the given value.
   
      :param number value: The value to update.

   .. method:: start_duration()

      Start a time measurement.

   .. method:: update_duration()

      Measure the time between now and the last call to :func:`update_duration` or :func:`start_duration` and update the timer.

   .. method:: finish_duration()

      Measure the time between now and the last call to :func:`update_duration` or :func:`start_duration` and update the timer. No further measurements can be done before calling :func:`start_duration` again.
