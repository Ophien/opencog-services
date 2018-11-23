![singnetlogo](assets/singnet-logo.jpg 'SingularityNET')

[singularitynet-home]: https://www.singularitynet.io
[contribution-guidelines]: https://github.com/singnet/wiki/blob/master/guidelines/CONTRIBUTING.md
[cpp-tutorial]: https://github.com/singnet/wiki/tree/master/tutorials/howToWriteCPPService
[opencog-tutorial]: https://github.com/singnet/wiki/tree/master/tutorials/howToWriteOpencogService
[opencog-pattern-miner]: https://wiki.opencog.org/w/Pattern_miner
[users-guide]: https://singnet.github.io/opencog-services/


[![CircleCI](https://circleci.com/gh/singnet/opencog-services.svg?style=svg)](https://circleci.com/gh/singnet/opencog-services)

# Opencog services to SingularityNET

This repository contains a number of SingularityNET services which uses Opencog resources.

Actually, we have only one C++ gRPC service which is used to call any of the
implemented services passing a command name and a list of arguments.

C++ and Scheme services are implemented in `src/cpp-services` and
`src/scm-services` respectively.

- [Echo](docs/Echo.md) - a simple 'echo' which just outputs its input, used to test the architecture and to serve as example.
- [EchoScheme](docs/EchoScheme.md) - the same, but written in `Scheme`.
- [PatternMiner](docs/PatternMiner.md) - load a knowledge base and run [Opencog's PatternMiner][opencog-pattern-miner]

[HTML User's Guide Hub][users-guide]

## Getting Started

In addition to the services thenselves, this repository contains all the basic
code and scripts required to build and deploy the Opencog services.

### Building and testing services locally (without blockchain)

We suggest you to build in a docker container with all the required packages using `Docker/Dockerfile`.
Then run container and continue from its prompt.

```
$ git clone https://github.com/singnet/opencog-services.git
$ docker build -t opencog_service_dev opencog-services/Docker/
$ docker run --name OPENCOG_SERVICE_DEV -ti opencog_service_dev /bin/bash
```

Find `server` and `client` executables in `bin/`

You can run tests locally. Start a `server`

```
$ ./bin/server &
```

and make any number of requests using `client`

```
$ ./bin/client sync Echo foo bar
foo bar
```

The command line above is requesting the the execution of the service `Echo`,
passing two arguments `foo` and `bar`. The service just echoes the passed input.

The keyword `sync` before the service name sates that the call should be
synchronous. So the `client` command will wait for the service to finish before
returning. You can make an asynchronous call using

```
$ ./bin/client sync Echo foo bar
Task "Echo" started. See results in: http://54.203.198.53:7000/ServiceAsynchronousOutput/opencog/Echo-53171
```

In this case the service return an URL which can be acessed asynchronously to
get the results (when they are ready). This is specially useful when running
services that take too long to execute.

Results remain available for 24 hours after being delivered. Then they are
deleted.

### Writing a new Opencog service

[This tutorial][opencog-tutorial] explains how to write a new Opencog-based service.

### Contributing and Reporting Issues

Please read our [guidelines][contribution-guidelines] before
submitting issues or pull requests. 


## Authors

* **Andre Senna** - *Maintainer* - [SingularityNET][singularitynet-home]

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details
