# token.pc-smart-contract

The pct.token smart contract is designed to distribute shares.

# Dependencies

* eosio 2.0^
* eosio.cdt 1.6^
* cmake 3.5^

# Compiling

```
./build.sh -e /root/eosio/2.0 -c /usr/opt/eosio.cdt
```

# Deploying

```
cleos set contract <your_account> ./build/Release/rct.token token.pc.wasm token.pc.abi
```
