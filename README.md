# vmars
FIX Probe written in C for running on Linux systems.

## Building

After cloning and from within the vmars checkout from git
```
> mkdir build
> cd build
> cmake ..
> make
```

## Testing

To test it you will at least 4 terminal windows open.

### Terminal 1 (Server)

```
> nc -lk 9999
```

This will act as the 'server' for receiving messages from the 'client'.

### Terminal 2 (Monitoring)

```
> nc -lku 41000
```

This should show the latency counters being printed.  If this stop showing counters
at any point try stopping and restarting it.

### Terminal 3 (VMars probe)

From within the build directory created earlier
```
> sudo ./vmars -i lo -c 9999 -t 1 -h 127.0.0.1 -p 41000
```
The vmars tool needs to run as root to get permissions to access the socket in interesting ways.
### Terminal 4 (Client)
From within the build directory created earlier
```
> ../gendata.sh 127.0.0.1
```

This should push through a couple of messages over port 9999 such that they will be captured by the probe.
Terminal 3 should show something similar to the following:
```
vmars.latency.max 0 1492815235000 type=gauge
vmars.latency.50 0 1492815235000 type=gauge
vmars.latency.99 0 1492815235000 type=gauge
vmars.latency.9999 0 1492815235000 type=gauge
vmars.latency.mean 0 1492815235000 type=gauge
vmars.latency.max 547 1492815236000 type=gauge
vmars.latency.50 547 1492815236000 type=gauge
vmars.latency.99 547 1492815236000 type=gauge
vmars.latency.9999 547 1492815236000 type=gauge
vmars.latency.mean 546 1492815236000 type=gauge
vmars.latency.max 0 1492815237000 type=gauge
vmars.latency.50 0 1492815237000 type=gauge
vmars.latency.99 0 1492815237000 type=gauge
vmars.latency.9999 0 1492815237000 type=gauge
vmars.latency.mean 0 1492815237000 type=gauge
```