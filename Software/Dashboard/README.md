# Overview

The C2Sv1.1 Camera & Power Dashboard allows users to easily interface with the C2Sv1.1 Processor board to retrieve image and power data via USB.

The dashboard connects to C2Sv1.1 via COM port through a USB-C connection.

> [!IMPORTANT]
> A **USB data cable** must be used. If you are unsure of the cable type, try plugging your phone into your computer/laptop. If it is a data cable, it will recognise your phone as a device - if not, it is a non-data cable.

# Functionality

## Connect Port

Selects the COM port to interface over. You must select **C2Sv1.1**. Once connected, the status to the right will change to **Connected** in green. If disconnected, the status will change to **Disconnected** in red.

## Configuration Panel

Allows the user to modify certain configurations of the image sensor.

### Colour Mode

Specified the colour format the images are generated in.

### Enable Test Pattern

Produces the following test pattern.

![OV7670 Test Pattern](https://www.channel6.dk/mainsite/Downloads/Test%20patterns%20SD/thumbs/75%25%20EBU%20Color%20Bars-250px.jpg)

> [!NOTE]
> The Test Pattern is independent of all configurations except for Colour Mode and Resolution.

### Resolution

Specifies the pixel resolution of the image.

### JPEG Quality

A measure of how much JPEG compression is applied to the image. 0 = Maximum Compression, 100 = No Compression.

### AEC Register Value

Specifies the exposure time of the image. Lower values = shorter exposure, Larger = longer exposure.

### Gain

Specifies the analog gain applied to the image. Larger values = brighter image.

## Send & Capture

Packages the configuration and sends a single request to the board. The dashboard will then wait for image data from the board and reconstruct the image as packets arrive. Once the entire image is constructed in the canvas, the status will change from Processing to Rendered.

> [!NOTE]
> The canvas, the area where the image is generated, will remain unchanged until the page is reloaded, new image data is received, or if the resolution is changed.

## Streaming

Performs the same functionality as Send & Capture, except it instantly requests a new image once the previous has rendered.

> [!WARNING]
> The dashboard cannot handle dynamic changes to resolution. Should a resolution change be made, stop streaming first. If this mistake is made, you must re-load the page.

## Status Bar

Located above the canvas, the status bar informs the user of the current dashboard state.

| Status | Basic Description |
| :--- | :--- |
| Disconnected | No connection to C2S via COM port |
| Connected | Connected to C2S |
| Processing | Image Request sent OR constructing image on canvas |
| Rendered | Image completely constructed on canvas |

## CSA Power Reading

Displays the most recent power data received by the C2S board. Samples logged representing how many power data points have been logged in the current session.

> [!IMPORTANT]
> A session is begun/reset once the dashboard is opened/refreshed, and all data from previous sessions are lost.

## Data Handling

| Item | Basic Description |
| :--- | :--- |
| Save As | Downloads the current canvas image (default Downloads folder) |
| Copy Frame | Copes the current canvas image to the Clipboard |
| Export CSV | Downloads a .CSV of all power data from current session (default Downloads folder) |
| Set Folder | Specifies a folder on the user's computer where data will be saved to |
| Auto-Save Streams | Automatically saves rendered images to the specified folder and specifies download location of Power .CSV export |

> [!NOTE]
> A folder must be specified for Auto-Save Stream to save data.