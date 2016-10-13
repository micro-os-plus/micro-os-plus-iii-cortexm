# POSIX Architecture

An xPack with POSIX architecture specific files.

## Package

[POSIX Architecture](https://github.com/micro-os-plus/posix-arch) package details:

* type: [xcdl](http://xcdl.github.io)
* name: `ilg/posix-arch`
* git repo: https://github.com/micro-os-plus/posix-arch.git
* homepage: https://github.com/micro-os-plus/posix-arch
* latest archive: https://github.com/micro-os-plus/posix-arch/archive/xpack.zip

## Version

* 0.1.1

## Releases

* none

## Package maintainers

* [Liviu Ionescu](http://liviusdotnet.worldpress.com) [&lt;ilg@livius.net&gt;](mailto:ilg@livius.net)

## Content contributors

* [Liviu Ionescu](http://liviusdotnet.worldpress.com) [&lt;ilg@livius.net&gt;](mailto:ilg@livius.net)

## Support

* [all issues](https://github.com/micro-os-plus/posix-arch/issues)

## Dependencies

* none

## Keywords

* posix
* architecture
* portable

## License

* [spdx](http://spdx.org/licenses/): MIT

--- 
## Documentation

This xPack provides the implementation for the µOS++ scheduler, running on a synthetic POSIX platform, like macOS and GNU/Linux.

## Configuration

To include this µOS++ port in a project, consider the following details.

### Include folders

- `include` 
 
### Source folders

- `src` 

### Symbols

- `_XOPEN_SOURCE=700L`

## Limitations

When running on a synthetic POSIX platform, the µOS++ scheduler runs in cooperative mode only.

## Tests

None.


--- 
