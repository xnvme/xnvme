(sec-backends-common-async-thrpool)=

# Async. I/O via `thrpool`

...

## Environment variables

* `XNVME_BE_THRPOOL_ASYNC_NTHREADS`: the number of threads each queue
  starts, from 1 to 1023. The default is 4. A value outside that range fails
  the queue init with `EINVAL`.
