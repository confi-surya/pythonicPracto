#python stringTemplate.py
#if [[ $? = 0 ]]; then
#    echo "success"
#@else
#    echo "failure: $?"
#fi
echo "Going to Execute python"
if python stringTemplate.py; then
    echo "Executed successfully"
    echo "Exit code of 0, success"
else
    echo "Exit code of $?, failure"
fi
echo "Now Shell Executing"
