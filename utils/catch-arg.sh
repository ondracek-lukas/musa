# Spectrograph  Copyright (C) 2015  Lukáš Ondráček <ondracek.lukas@gmail.com>, see README file

# exit_on_unknown_param = (true|false)
# exit_on_missing_param = (true|false)
# remove_wanted_param = (true|false)
# wanted_param = --param

if [[ -z $exit_on_unknown_param ]]; then exit_on_unknown_param=false; fi
if [[ -z $exit_on_missing_param ]]; then exit_on_missing_param=true; fi
if [[ -z $remove_wanted_param ]]; then remove_wanted_param=true; fi


value=""
params=()
while [[ -n "$@" ]]; do
	if [[ "$1" = $wanted_param ]]; then
		if [[ -z "$2" ]]; then
			echo "Missing $wanted_param argument." >&2
			exit 1
		fi
		value="$2"
		if ! $remove_wanted_param; then
			params=("${params[@]}" "$1" "$2")
		fi
		shift 2
	elif $exit_on_unknown_param; then
		echo "Wrong argument $1." >&2
		exit 1
	else
		params=("${params[@]}" "$1")
		shift
	fi
done
if $exit_on_missing_param && [[ -z "$value" ]]; then
	echo "Parameter $wanted_param missing." >&2
	exit 1
fi
set -- "${params[@]}"
