for x in *; do if [ -n "$(tail -c 1 <"$x")" ]; then echo >>"$x"; fi; done
cat *.txt > all.all
sed -i "/^[^ ]*$/d" all.all
sed -i "/^.* $/d" all.all
R --slave -e 'mean(read.table("all.all")$V2)' | cut -d" " -f2