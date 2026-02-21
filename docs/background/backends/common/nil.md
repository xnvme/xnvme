(sec-backends-common-async-nil)=

# Async. I/O via `nil`

The purpose of the `nil` backend is entirely for debugging and measuring the
IO-layer. The `nil` asynchronous implementation only queues up commands and when
polled for completion, commands are returned with success.
