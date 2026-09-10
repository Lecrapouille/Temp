# How to debug OpenGL

## GPU debug output

The `gpu::` layer installs a debug callback when the driver supports
`KHR_debug` (guaranteed in 4.3+). Warnings and errors from the driver come
out on `stderr`, prefixed with `[gpu] warning:` or `[gpu] error:`, alongside
the API message id and the actual sentence. The gallery routinely shows
those; a real error typically comes with a `gpu::Result` failure a few lines
above, and the two together are the whole story.

    10|## apitrace

For call-by-call inspection of what actually reached the driver:

```sh
apitrace trace --api gl build/OpenGLCppWrapper-examples 05_IndexedCube
qapitrace OpenGLCppWrapper-examples.trace
```

Older traces stack up next to the binary. Delete the previous one, or
apitrace will suffix the new file with `.1.trace`, `.2.trace`, and so on.
    20|
See [apitrace](https://github.com/apitrace/apitrace) for the tool itself.

## RenderDoc

RenderDoc works too, and gives a per-draw view of state, shaders and the
attached targets. Point it at `build/OpenGLCppWrapper-examples`; the
gallery's ImGui overlay is drawn by ImGui's own backend after the library's
own state has been flushed with `gpu::forgetRenderState()`, so it does not
interfere with what RenderDoc captures for the example itself.
