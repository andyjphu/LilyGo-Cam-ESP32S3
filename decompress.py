import re
import gzip

def decompress_html():
    try:
        with open('/Users/andyphu/Smartglasses/LilyGo-Cam-ESP32S3/examples/OCRCamera/camera_index.h', 'r') as f:
            content = f.read()

        # Find the byte array for ov2640
        match = re.search(r'const uint8_t index_ov2640_html_gz\[\] = \{(.*?)\};', content, re.DOTALL)
        if not match:
            print("Could not find the byte array in the header file.")
            return

        hex_values = match.group(1)
        # Clean up and split the hex values
        hex_values = hex_values.replace('\n', '').replace('\r', '')
        byte_strings = [val.strip() for val in hex_values.split(',') if val.strip()]
        
        # Convert hex strings to bytes
        gzipped_data = bytes([int(b, 16) for b in byte_strings])

        # Decompress the data
        decompressed_data = gzip.decompress(gzipped_data)

        # Print the HTML
        print(decompressed_data.decode('utf-8'))

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == '__main__':
    decompress_html()
