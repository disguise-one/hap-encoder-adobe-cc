<header>
<div position="absolute" left="0" top="0" style="background-color:#33272C;">
<a href="http://hap.video"><img src="../../asset/hap-logo.svg" width="20%" height="auto"></a>
</div>
</header>

# Exporter for Adobe CC 2018

## Introduction

This is the community-supplied Hap and Hap Q exporter plugin for Adobe CC 2018.

HAP is a collection of high-performance codecs optimised for playback of multiple layers of video.

## Getting it

Releases of this plugin are available at

    https://github.com/GregBakker/hap-exporter-adobe-cc/releases

## Requirements

This codec has been tested on Windows 10, and supports Core i3 processors and later.

It has been tested in Adobe Media Encoder CC 2018 12.1.2.69

## Installation

Obtain the installer executable and run it.

## Usage

After installation, the encoders will be available under the HAP format in Adobe Media Encoder CC 2018 and in Adobe Premiere CC 2018.

TODO - format pic

Presets are supplied

TODO - presets pic

or you can also setup your own customisations.

TODO - customised pic

Movies that are encoded with the plugin are exported into .mov files.

### Choosing the right codec for the job: Hap, Hap Alpha, Hap Q and Hap Q Alpha

There are four different flavors of HAP to choose from when encoding your clips.


Hap

:   lowest data-rate and reasonable image quality

Hap Alpha

:   same image quality as Hap, and supports an Alpha channel

Hap Q

:   improved image quality, at the expense of larger file sizes

Hap Q Alpha

:   improved image quality and an Alpha channel, at the expense of larger file sizes

### Codec parameters

An optional specified 'chunk' size may be specified to optimize for ultra high resolution video on a particular hardware system. This setting should typically only be used if you are reaching a CPU performance bottleneck during playback. As a general guide, for HD footage or smaller you can set the chunk size to 1 and for 4k or larger footage the number of chunks should never exceed the number of CPU cores on the computer used for playback.

### Playback

Playback requires support for the HAP codec. There are plenty of options, including

-  the HAP quicktime codec
-  VLC
-  many media servers
-  HAP codec ingestion in Adobe products

## What is HAP

HAP is a collection of high-performance codecs optimised for playback of multiple layers of video.

HAP prioritises decode-speed, efficient upload to GPUs and GPU-side decoding to enable the highest amount of video content to be played back at once on modern hardware.

Please see

    http://hap.video/

for details.

## Known issues

TODO

## Credits

Principal authors of this plugin are

-  Greg Bakker (gbakker@gmail.com)
-  Richard Sykes

Development of this plugin was sponsored by disguise, makers of the disguise show production software and hardware
    http://disguise.one

The Hap codec was developed by Tom Butterworth with the support of VIDVOX.

Many thanks to Tom Butterworth, Dave Lublin, Nick Wilkinson, Ruben Garcia and the disguise QA team for their assistance.



