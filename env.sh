if ! command -v realpath &> /dev/null; then
  export alias realpath="readlink -f"
fi

# shellcheck disable=SC2128
# shellcheck disable=SC2030
script_source=$([[ -z "$BASH_SOURCE" ]] && echo "$0" || echo "$BASH_SOURCE")
script_file=$(realpath "${script_source}")
script_dir=$(dirname -- "${script_file}")

cosmo_version=${COSMO_VERSION:-4.0.2}

cosmo_dir="${script_dir}/cosmocc-${cosmo_version}"

if [[ ! -d $cosmo_dir ]]; then
  mkdir "$cosmo_dir"
  pushd "$cosmo_dir"
  wget "https://github.com/jart/cosmopolitan/releases/download/${cosmo_version}/cosmocc-${cosmo_version}.zip"
  unzip "cosmocc-${cosmo_version}.zip"
  popd
fi

export PATH="${cosmo_dir}/bin:$PATH"
