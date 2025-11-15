## Repozytorium Gitea

Creating a new repository on the command line

Existing repositories

git remote remove origin

git config --global credential.helper store

git remote add origin https://dlb.blue/drosak1/SALGSM.git

git remote set-url origin https://[USER]:[TOKEN]@dlb.blue/drosak/SALGSM.git

git push --force origin main


## Github

git init & add files & commit

git remote add origin https://github.com/drosak1/SALGSM.git

git pull origin main --allow-unrelated-histories

https://github.com/settings/tokens -> “Personal access tokens (classic)”

 repo [x] workflow [x] read:org

Username for 'https://github.com': drosak1

Password for 'https://github.com':

git push --force origin main


## Zwiększenie buffora gita

git config --global core.sshCommand "ssh -o Compression=no -o TCPKeepAlive=yes -o ServerAliveInterval=30 -o ServerAliveCountMax=10"

git config --global http.postBuffer 524288000

## Autor
Dawid Rosak
