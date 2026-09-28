# Build without a model

We have an accepted body and have inspected it. The next build should not need
credentials, a network connection, or another generation bill.

From the same directory, rebuild the original source in offline mode:

```sh
llmc++ -fllm-offline main.cpp -o scores
./scores
```

The compiler loads a matching body from `.llmcache/`. If there is no matching
entry, it fails rather than silently contacting a backend. This makes offline
mode useful when you want CI to build only previously accepted bodies.

## Change the instruction

Change the prompt in `main.cpp` to request lowest-to-highest order and run the
offline command again. It should fail: the old body belongs to a different
instruction. Now rebuild without offline mode:

```sh
llmc++ main.cpp -o scores
./scores
```

Review the new body just as before. Source context, included headers, generation
options, and system instructions can also invalidate a cached body, even when
you did not change the prompt itself. A cache hit is reuse, not a correctness
guarantee.

You now have the complete loop: describe, generate, review, and reuse. For your
own project, continue with [driver options and caching](../how_to_use_driver_options_and_caching.md)
or [custom agents](../how_to_write_an_agent.md).
