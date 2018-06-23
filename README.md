# xcompmgr_slim

A fork of  `xcompmgr`  which only contains the `-a` mode, which ensencially cuts out the majority of `xcompmgr` (from ~2k lines to ~150).

## Compile

```bash
gcc -Os -Wall xcompmgr.cpp -lXcomposite -lX11 -o xcompmgr_slim
```



