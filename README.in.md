## Purpose

This xPack provides the implementation for the µOS++ scheduler, running on synthetic POSIX platforms, like **macOS** and **GNU/Linux**.

## Status

The µOS++ synthetic POSIX scheduler port is fully functional.

## Build Configuration

To include this µOS++ port in a project, consider the following details.

### Include folders

- `include` 
 
### Source folders

- `src` 

### Symbols

- `_XOPEN_SOURCE=700L`

## Limitations

When running on a synthetic POSIX platform, the µOS++ scheduler runs in cooperative mode only.

## Future enhancements

If possible, it would be useful to run the scheduler on Windows, using the minGW-W64 development environment.

## Tests

None.

