const epd2in7b = require('./build/Release/epd2in7b');
const gd = require('node-gd');

const button1 = 5
const button2 = 6
const button3 = 13
const button4 = 19

const rpi_gpio_buttons = require('rpi-gpio-buttons')

const buttons = new Promise((resolve) => {
	let handler = rpi_gpio_buttons(
		[button1, button2, button3, button4],
		{ mode: rpi_gpio_buttons.MODE_BCM });
	resolve(handler);
})

const width = epd2in7b.width();
const height = epd2in7b.height();

function getImageBuffer(orientation) {
	return new Promise(resolve => {
		let img
		if (orientation === undefined || orientation === 'portrait') {
			img = gd.createSync(width, height);
		} else {
			img = gd.createSync(height, width);
		}

		for (let i = 0; i < 64; i++)
			img.colorAllocate(0, 0, 0);
		for (let i = 64; i < 192; i++)
			img.colorAllocate(0, 0, 255);
		for (let i = 192; i < 256; i++)
			img.colorAllocate(255, 0, 0);

		return resolve(img);
	})
}

function displayImageBuffer(img) {
	return new Promise(resolve => {
		let bufBlack = new Buffer.alloc(width * height, 0);
		let bufRed = new Buffer.alloc(width * height, 0);
		let hasBlack = false;
		let hasRed = false;

		for(let y = 0; y < height; y++) {
			for(let  x = 0; x < width; x++) {
				let color = img.height == height
					? img.getPixel(x, y)
					: img.getPixel(img.width - y, x);
				if (color < 64) { //white
					bufBlack[ x + y * width ] = 0x00;
					bufRed[ x + y * width ] = 0x00;
				} else if (color < 192) { //black
					hasBlack = true;
					bufBlack[ x + y * width ] = 0xff;
					bufRed[ x + y * width ] = 0x00;
				} else { // red
					hasRed = true;
					bufBlack[ x + y * width ] = 0x00;
					bufRed[ x + y * width ] = 0xff;
				}
			}
		}
		epd2in7b.displayFrame(
			bufBlack, //hasBlack ? bufBlack : null,
			bufRed, //hasRed ? bufRed : null,
			() => {
				resolve();
			}
		);
	})
}

exports.getImageBuffer = getImageBuffer;

exports.displayImageBuffer = displayImageBuffer;

exports.init = options => new Promise(resolve => {
	epd2in7b.init(options, () => {
		resolve();
	});
})

exports.clear = () => new Promise(resolve => {
	epd2in7b.clear(() => {
		resolve();
	});
})

exports.sleep = () => new Promise(resolve => {
	epd2in7b.sleep(() => {
		resolve();
	});
})

exports.colors = {
	white: 0,
	black: 128,
	red: 255,
}

exports.width = width;

exports.height = height;

exports.gd = gd;

exports.buttons = {
	button1: button1,
	button2: button2,
	button3: button3,
	button4: button4,
	handler: buttons
}
