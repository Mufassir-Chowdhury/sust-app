#!/bin/sh
# Fetch the scheme sources used by tools/import_schemes.py, pinned to the commits the
# current schemes/ were built from.  Usage: tools/fetch_scheme_sources.sh DEST_DIR
# Then:  python3 -I tools/import_schemes.py DEST_DIR schemes
#        python3 tools/scheme_check.py --index schemes
set -eu
dest=${1:?usage: $0 DEST_DIR}
mkdir -p "$dest"

fetch() {  # dir url sha [sparse patterns...]
  dir=$1 url=$2 sha=$3; shift 3
  if [ -d "$dest/$dir/.git" ]; then echo "$dir: exists, skipping"; return; fi
  git init -q "$dest/$dir"
  git -C "$dest/$dir" remote add origin "$url"
  if [ $# -gt 0 ]; then
    git -C "$dest/$dir" config core.sparseCheckout true
    printf '%s\n' "$@" > "$dest/$dir/.git/info/sparse-checkout"
    git -C "$dest/$dir" fetch -q --depth 1 --filter=blob:none origin "$sha"
  else
    git -C "$dest/$dir" fetch -q --depth 1 origin "$sha"
  fi
  git -C "$dest/$dir" checkout -q FETCH_HEAD
  echo "$dir: $(git -C "$dest/$dir" rev-parse HEAD)"
}

fetch fastmatmul_git https://github.com/arbenson/fast-matmul.git 132358be7a6ab57c83dd5ce70ece460bd1ec5dfa
fetch jgdumas_plinopt https://github.com/jgdumas/plinopt.git c45a7cd4486050f9741f4712bd41b688de9c3e7d
fetch google-deepmind_alphatensor https://github.com/google-deepmind/alphatensor.git 1949163da3bef7e3eb268a3ac015fd1c2dbfc767
fetch google-deepmind_alphaevolve_results https://github.com/google-deepmind/alphaevolve_results.git 4226acbf237ff9ad10ba7673a2af127a2d8a5971
fetch mkauers_matrix-multiplication https://github.com/mkauers/matrix-multiplication.git 12c26b29a5458e173813911fb4f2c2865fba841e
fetch jakobmoosbauer_flips https://github.com/jakobmoosbauer/flips.git e31a0a0f0d2577cee5da047ca7dcae0c61992e40
# the full Perminov repository is ~3.7 GB; only the small formats are needed
fetch perminov https://github.com/dronperminov/FastMatrixMultiplication.git 64f58a5e40806bc47847b11dd8aceec043fa895d \
  /schemes/status.json \
  /schemes/known/ \
  '!/schemes/known/tensor/' \
  '/schemes/known/tensor/[2-6]x[2-6]x[2-6]_tensor.mpl' \
  /schemes/results/addition_reduced_ZT/ \
  /schemes/results/naive_addition_reduced_ZT/ \
  /schemes/results/serendipitous_base/ \
  '/schemes/results/*/[2-6]x[2-6]x[2-6]_*'
