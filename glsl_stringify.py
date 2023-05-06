import os
import time

glsl_folder = "code/renderergl2/glsl"
output_folder = "build/dynamic/renderergl2"

# create the output folder if it doesn't exist
if not os.path.exists(output_folder):
    os.makedirs(output_folder)

for filename in os.listdir(glsl_folder):
    if filename.endswith(".glsl"):
        input_path = os.path.join(glsl_folder, filename)
        output_path = os.path.join(output_folder, filename.replace(".glsl", ".c"))
        if os.path.exists(output_path) and os.path.getmtime(output_path) >= os.path.getmtime(input_path):
            print(f"Skipping {output_path} (already up-to-date)")
            continue
        print(f"Processing {input_path}...")
        with open(input_path, 'r') as infile, open(output_path, 'w') as outfile:
            outfile.write("const char *fallbackShader_" + os.path.splitext(filename)[0] + " =\n")
            for line in infile:
                line = line.replace("\\", "\\\\")
                line = line.replace("\t", "\\t")
                line = line.replace('"', '\\"')
                line = '"' + line.rstrip('\n') + '\\n"' + '\n'
                outfile.write(line)
            outfile.write(";")
        print(f"Created {output_path}\nSUCCESS!")
