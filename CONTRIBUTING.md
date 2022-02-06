# Contribution Guideline
We love your input! We want to make contributing to this project as easy and transparent as possible, whether it's:

- Reporting a bug
- Discussing the current state of the code
- Submitting a fix
- Proposing new features
- Becoming a maintainer

## Development Workflow
1. Fork this repo and create your development branch from `master`.
2. Add tests and make sure all implementations are covered by the tests.
3. Regenerate and checks the single header file by running these commands from the root folder.
```sh
make amalgamate
make check-amalgamation
make check-single-includes
```
4. Issue that pull request!

## Any contributions you make will be under the MIT Software License
In short, when you submit code changes, your submissions are understood to be under the same [MIT License](http://choosealicense.com/licenses/mit/) that covers the project. Feel free to contact the maintainers if that's a concern.

## Report bugs using Github's [issues](https://github.com/nadjieb/cpp-mjpeg-streamer/issues)
We use GitHub issues to track public bugs. Report a bug by [opening a new issue](https://github.com/nadjieb/cpp-mjpeg-streamer/issues/new/choose); it's that easy!

## Write bug reports with detail, background, and sample code

**Great Bug Reports** tend to have:

- A quick summary and/or background
- Steps to reproduce
  - Be specific!
  - Give sample code if you can.
- What you expected would happen
- What actually happens
- Notes (possibly including why you think this might be happening, or stuff you tried that didn't work)

People *love* thorough bug reports. I'm not even kidding.

## License
By contributing, you agree that your contributions will be licensed under its MIT License.
