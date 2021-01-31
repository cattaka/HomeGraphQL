## What is this
These are the configuration files for the home GraphQL server.
It is assumed to be installed in a local network.
It is used to aggregate data from home-made IoT devices such as temperature sensors and humidity sensors.

## How it work
This uses [PostGraphile](https://www.graphile.org/).
The the configuration files are in `dd` directory.

## Run PostGraphile on local docker
Read [the official documents](https://github.com/graphile/postgraphile) for details.

Prepare these environment variables.
```
POSTGRES_USER
POSTGRES_PASSWORD
POSTGRES_HOST
POSTGRES_PORT
POSTGRES_DATABASE
POSTGRES_SCHEMA
JWT_SECRET
```

Then, execute the following commands.

```bash
$ docker pull graphile/postgraphile:v4.10.0
$ docker run --init graphile/postgraphile:v4.10.0 --help
$ docker run --init -p 5000:5000 graphile/postgraphile:v4.10.0 \
  --connection postgres://$POSTGRES_USER:$POSTGRES_PASSWORD@$POSTGRES_HOST:$POSTGRES_PORT/$POSTGRES_DATABASE \
  --schema $POSTGRES_SCHEMA \
  --watch \
  --jwt-token-identifier jwt_token \
  --jwt-secret $JWT_SECRET
```
