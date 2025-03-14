# This sketch will take the output files from the West Lab Hydrophone Data Logger and convert each .RAW output into a .wav file type.
# A path to the folder containing the data files is given(path_data_folder). Each of the output files stored in the folder at the path will be converted and stored in a new folder.

import wave
import struct
import os

# path_data_folder should be the path folder that contains the data loggers "RECORD.X.X-X-X-X-X-X-X-X.RAW" files.
# At this path a folder named "Converted Files" will be created and all converted filed will be stored within.
path_data_folder = r'C:\Users\mitch\OneDrive\SchoolFiles\_West\USGS Box\Data_Logger\Testing_Data\Sample Records' 

def convert_raw_to_wav(raw_file, wav_file, channels=1, sample_width=2, sample_rate=44100):
    """Converts a single RAW file to a WAV file."""
    try:
        with open(raw_file, 'rb') as f:
            raw_data = f.read()

        with wave.open(wav_file, 'wb') as wf:
            wf.setnchannels(channels)
            wf.setsampwidth(sample_width)
            wf.setframerate(sample_rate)
            wf.writeframes(raw_data)
        print(f"Converted {raw_file} to {wav_file}")
    except Exception as e:
        print(f"Error converting {raw_file}: {e}")

def convert_all_raw_to_wav(path, file_type='.RAW'):
    """
    Converts all RAW files in the given path to WAV files 
    and saves them in a "Converted Files" folder.
    """
    output_folder = os.path.join(path, "Converted Files")  # Create output folder path
    os.makedirs(output_folder, exist_ok=True)  # Create the folder if it doesn't exist

    for root, _, files in os.walk(path):
        for filename in files:
            if filename.endswith(file_type):
                raw_file = os.path.join(root, filename)
                wav_file = os.path.join(output_folder, os.path.splitext(filename)[0] + '.wav') 
                convert_raw_to_wav(raw_file, wav_file)

if __name__ == "__main__":
    convert_all_raw_to_wav(path_data_folder)
