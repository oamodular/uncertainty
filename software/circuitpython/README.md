# Running the scripts in this directory

Once you flash your Uncertainty with the most recent version of CircuitPython for RP2040, you should see `CIRCUITPY` show up as a disk on your computer. Copy the `examples` and `lib` from this directory into the `CIRCUITPY` drive. Then take make a copy of whatever script your want to run. Name the copy `code.py` and move it to the root level of `CIRCUITPY`. Now you’re running it!

# Making a .uf2 File

You’ll have to make a .uf2 file if you want others to be able to easily install what you have on your `CIRCUITPY`. To do this, install `picotool`. If you have have `brew`, just do a `brew install picotool`. Now run the command `picotool save --all ExampleNameHere.uf2`. Now you can pass this to your friend like a hacker.