# About

The PdServ library provides process data communication mechanisms for Linux realtime applications in user space.
It comes from EtherLab
https://www.etherlab.org/en/pdserv

This work is based on the original mecurial repository
http://hg.code.sf.net/p/pdserv/code

* The original mercurial repository has been cloned and converted to git
with `hg-fast-export`
```bash
hg clone http://hg.code.sf.net/p/pdserv/code pdserv-hg
git init pdserv-git && cd pdserv-git
hg-fast-export -r ../pdserv-hg --force && git checkout HEAD
```
* Then I have started to develop my own improvements in a new branch.
* Sometimes my patches may be accepted and merged in the main mercurial repository
* I will try to synchronize regularly with the original code.
* The last synchronisation occurs on 2018-11-21 with mecurial revision 528:90fefe8b0e99
