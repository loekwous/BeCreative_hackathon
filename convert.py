import wave
import matplotlib.pyplot as plt

files = ["bird", "chainsaw", "fire", "whale"]

out_list = []
plot_list = []
x_list = []
for i in range(len(files)):
  out_list.append([])
  x_list.append([])
  plot_list.append([])
n_frames = 0

frame_division = 16

header = [
      "#ifndef SOUNDS_H\n",
      "#define SOUNDS_H\n\n",
     ]

footer = [
  "#endif\n"
    ]

index = 0

if __name__ == '__main__':
  header_file = open("sounds.h", "w")
  header_file.writelines(header)
  header_file.write("enum class EStates {None, ")
  for sound in files:
    header_file.write(sound.capitalize())
    if sound != files[-1]:
      header_file.write(", ")
  header_file.write("};\nvolatile EStates state = EStates::None;\n\n")
  header_file.flush()
  for sound in files:
    with wave.open(sound + ".wav", "rb") as f:
      n_frames = f.getnframes()
      print(sound)
      print("frame rate: " + str(f.getframerate()))
      print("channels:   " + str(f.getnchannels()))
      print("frames:     " + str(f.getnframes()))
      print("len frames: " + str(f.getsampwidth()))
      for i in range(f.getnframes()):
        val = int.from_bytes(f.readframes(1),'little', signed=True)
        if i % frame_division == 0:
          out_list[index].append(val)
          plot_list[index].append(val + (60000 * index))
          x_list[index].append(i/frame_division)

    n_frames = len(out_list[index])
    max_value = max(out_list[index])
    min_value = min(out_list[index])

    header_item = [
      "const int16_t size" + str(sound).capitalize() + " = " + str(n_frames) + ";\n",
      "const int16_t max" + str(sound).capitalize() + " = " + str(max_value) + ";\n",
      "const int16_t min" + str(sound).capitalize() + " = " + str(min_value) + ";\n",
      "const int16_t frameDivision" + str(sound).capitalize() + " = " + str(frame_division) + ";\n",
      "int16_t " + str(sound).capitalize() + "[" + str(n_frames) + "]{\n"
    ]

    header_file.writelines(header_item)
    header_file.flush()
    for i in range(len(out_list[index])):
      header_file.write(str(out_list[index][i]))
      if i < len(out_list[index])-1:
        header_file.write(",")
      if i % 10 == 9:
        header_file.write("\n")
    header_file.write("};\n")
    header_file.flush()
    index += 1

  header_file.writelines(footer)
  header_file.close()

  for i in range(len(files)):
    plt.plot(x_list[i], plot_list[i])
  plt.show()
