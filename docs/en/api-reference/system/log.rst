Logging library
===============

:link_to_translation:`zh_CN:[中文]`

Overview
--------

The logging library provides three ways for setting log verbosity:

- **At compile time**: in menuconfig, set the verbosity level using the option :ref:`CONFIG_LOG_DEFAULT_LEVEL`.
- Optionally, also in menuconfig, set the maximum verbosity level using the option :ref:`CONFIG_LOG_MAXIMUM_LEVEL`. By default, this is the same as the default level, but it can be set higher in order to compile more optional logs into the firmware.
- **At runtime**: all logs for verbosity levels lower than :ref:`CONFIG_LOG_DEFAULT_LEVEL` are enabled by default. The function :cpp:func:`esp_log_level_set` can be used to set a logging level on a per-module basis. Modules are identified by their tags, which are human-readable ASCII zero-terminated strings. Note that the ability to change the log level at runtime depends on :ref:`CONFIG_LOG_DYNAMIC_LEVEL_CONTROL`.
- **At runtime**: if :ref:`CONFIG_LOG_MASTER_LEVEL` is enabled then a ``Master logging level`` can be set using :cpp:func:`esp_log_set_level_master`. This option adds an additional logging level check for all compiled logs. Note that this will increase application size. This feature is useful if you want to compile a lot of logs that are selectable at runtime, but also want to avoid the performance hit from looking up the tags and their log level when you don't want log output.

There are the following verbosity levels:

- Error (lowest)
- Warning
- Info
- Debug
- Verbose (highest)

.. note::

    The function :cpp:func:`esp_log_level_set` cannot set logging levels higher than specified by :ref:`CONFIG_LOG_MAXIMUM_LEVEL`. To increase log level for a specific file above this maximum at compile time, use the macro `LOG_LOCAL_LEVEL` (see the details below).


How to Use Logging Library
--------------------------

In each C file that uses logging functionality, define the TAG variable as shown below:

.. code-block:: c

    static const char* TAG = "MyModule";

Then use one of logging macros to produce output, e.g:

.. code-block:: c

    ESP_LOGW(TAG, "Baud rate error %.1f%%. Requested: %d baud, actual: %d baud", error * 100, baud_req, baud_real);

Several macros are available for different verbosity levels:

* ``ESP_LOGE`` - Error (lowest)
* ``ESP_LOGW`` - Warning
* ``ESP_LOGI`` - Info
* ``ESP_LOGD`` - Debug
* ``ESP_LOGV`` - Verbose (highest)

Additionally, there are ``ESP_EARLY_LOGx`` versions for each of these macros, e.g. :c:macro:`ESP_EARLY_LOGE`. These versions have to be used explicitly in the early startup code only, before heap allocator and syscalls have been initialized. Normal ``ESP_LOGx`` macros can also be used while compiling the bootloader, but they will fall back to the same implementation as ``ESP_EARLY_LOGx`` macros.

There are also ``ESP_DRAM_LOGx`` versions for each of these macros, e.g. :c:macro:`ESP_DRAM_LOGE`. These versions are used in some places where logging may occur with interrupts disabled or with flash cache inaccessible. Use of this macros should be as sparse as possible, as logging in these types of code should be avoided for performance reasons.

.. note::

    Inside critical sections interrupts are disabled so it's only possible to use ``ESP_DRAM_LOGx`` (preferred) or ``ESP_EARLY_LOGx``. Even though it's possible to log in these situations, it's better if your program can be structured not to require it.

To override default verbosity level at file or component scope, define the ``LOG_LOCAL_LEVEL`` macro.

At file scope, define it before including ``esp_log.h``, e.g.:

.. code-block:: c

    #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
    #include "esp_log.h"

At component scope, define it in the component CMakeLists:

.. code-block:: cmake

    target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")

Dynamic Log Level Control
-------------------------

To configure logging output per module at runtime, add calls to the function :cpp:func:`esp_log_level_set` as follows:

.. code-block:: c

   esp_log_level_set("*", ESP_LOG_ERROR);        // set all components to ERROR level
   esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack
   esp_log_level_set("dhcpc", ESP_LOG_INFO);     // enable INFO logs from DHCP client

.. note::

    The "DRAM" and "EARLY" log macro variants documented above do not support per module setting of log verbosity. These macros will always log at the "default" verbosity level, which can only be changed at runtime by calling ``esp_log_level("*", level)``.

Even when logs are disabled by using a tag name, they will still require a processing time of around 10.9 microseconds per entry.

The log component provides several options to better adjust the system to your needs, reducing memory usage and speeding up operations. The :ref:`CONFIG_LOG_TAG_LEVEL_IMPL` option sets the method of tag level checks:

- "None". This option disables the ability to set the log level per tag. The ability to change the log level at runtime depends on :ref:`CONFIG_LOG_DYNAMIC_LEVEL_CONTROL`. If disabled, changing the log level at runtime using :cpp:func:`esp_log_level_set` is not possible. This implementation is suitable for highly constrained environments.
- "Linked list" (no cache). This option enables the ability to set the log level per tag. This approach searches the linked list of all tags for the log level, which may be slower for a large number of tags but may have lower memory requirements than the cache approach.
- (Default) "Cache + Linked List". This option enables the ability to set the log level per tag. This hybrid approach offers a balance between speed and memory usage. The cache stores recently accessed log tags and their corresponding log levels, providing faster lookups for frequently used tags.

When the :ref:`CONFIG_LOG_DYNAMIC_LEVEL_CONTROL` option is enabled, log levels to be changed at runtime via :cpp:func:`esp_log_level_set`. Dynamic log levels increase flexibility but also incurs additional code size.
If your application does not require dynamic log level changes and you do not need to control logs per module using tags, consider disabling :ref:`CONFIG_LOG_DYNAMIC_LEVEL_CONTROL`. It reduces IRAM usage by approximately 260 bytes, DRAM usage by approximately 264 bytes, and flash usage by approximately 1 KB compared to the default option. It is not only streamlines logs for memory efficiency but also contributes to speeding up log operations in your application about 10 times.

.. note::

    The "Linked list" and "Cache + Linked List" options will automatically enable :ref:`CONFIG_LOG_DYNAMIC_LEVEL_CONTROL`.

Master Logging Level
^^^^^^^^^^^^^^^^^^^^

To enable the Master logging level feature, the :ref:`CONFIG_LOG_MASTER_LEVEL` option must be enabled. It adds an additional level check for ``ESP_LOGx`` macros before calling :cpp:func:`esp_log_write`. This allows to set a higher :ref:`CONFIG_LOG_MAXIMUM_LEVEL`, but not inflict a performance hit during normal operation (only when directed). An application may set the master logging level (:cpp:func:`esp_log_set_level_master`) globally to enforce a maximum log level. ``ESP_LOGx`` macros above this level will be skipped immediately, rather than calling :cpp:func:`esp_log_write` and doing a tag lookup. It is recommended to only use this in an top-level application and not in shared components as this would override the global log level for any user using the component. By default, at startup, the Master logging level is :ref:`CONFIG_LOG_DEFAULT_LEVEL`.

Note that this feature increases application size because the additional check is added into all ``ESP_LOGx`` macros.

The snippet below shows how it works. Setting the Master logging level to ``ESP_LOG_NONE`` disables all logging globally. :cpp:func:`esp_log_level_set` does not currently affect logging. But after the Master logging level is released, the logs will be printed as set by :cpp:func:`esp_log_level_set`.

.. code-block:: c

    // Master logging level is CONFIG_LOG_DEFAULT_LEVEL at start up and = ESP_LOG_INFO
    ESP_LOGI("lib_name", "Message for print");          // prints a INFO message
    esp_log_level_set("lib_name", ESP_LOG_WARN);        // enables WARN logs from lib_name

    esp_log_set_level_master(ESP_LOG_NONE);             // disables all logs globally. esp_log_level_set has no effect at the moment

    ESP_LOGW("lib_name", "Message for print");          // no print, Master logging level blocks it
    esp_log_level_set("lib_name", ESP_LOG_INFO);        // enable INFO logs from lib_name
    ESP_LOGI("lib_name", "Message for print");          // no print, Master logging level blocks it

    esp_log_set_level_master(ESP_LOG_INFO);             // enables all INFO logs globally

    ESP_LOGI("lib_name", "Message for print");          // prints a INFO message

Logging to Host via JTAG
^^^^^^^^^^^^^^^^^^^^^^^^

By default, the logging library uses the vprintf-like function to write formatted output to the dedicated UART. By calling a simple API, all log output may be routed to JTAG instead, making logging several times faster. For details, please refer to Section :ref:`app_trace-logging-to-host`.

Thread Safety
^^^^^^^^^^^^^

The log string is first written into a memory buffer and then sent to the UART for printing. Log calls are thread-safe, i.e., logs of different threads do not conflict with each other.


Application Example
-------------------

The logging library is commonly used by most ESP-IDF components and examples. For demonstration of log functionality, check ESP-IDF's :idf:`examples` directory. The most relevant examples that deal with logging are the following:

* :example:`system/ota`
* :example:`storage/sd_card`
* :example:`protocols/https_request`

API Reference
-------------

.. include-build-file:: inc/esp_log.inc
.. include-build-file:: inc/esp_log_level.inc
.. include-build-file:: inc/esp_log_buffer.inc
.. include-build-file:: inc/esp_log_timestamp.inc
.. include-build-file:: inc/esp_log_color.inc
.. include-build-file:: inc/esp_log_write.inc
