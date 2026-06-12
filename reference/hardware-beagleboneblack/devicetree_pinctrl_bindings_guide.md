== Introduction ==

Hardware modules that control pin multiplexing or configuration parameters
such as pull-up/down, tri-state, drive-strength etc are designated as pin
controllers. Each pin controller must be represented as a node in device tree,
just like any other hardware module.

Hardware modules whose signals are affected by pin configuration are
designated client devices. Again, each client device must be represented as a
node in device tree, just like any other hardware module.

For a client device to operate correctly, certain pin controllers must
set up certain specific pin configurations. Some client devices need a
single static pin configuration, e.g. set up during initialization. Others
need to reconfigure pins at run-time, for example to tri-state pins when the
device is inactive. Hence, each client device can define a set of named
states. The number and names of those states is defined by the client device's
own binding.

The common pinctrl bindings defined in this file provide an infrastructure
for client device device tree nodes to map those state names to the pin
configuration used by those states.

Note that pin controllers themselves may also be client devices of themselves.
For example, a pin controller may set up its own "active" state when the
driver loads. This would allow representing a board's static pin configuration
in a single place, rather than splitting it across multiple client device
nodes. The decision to do this or not somewhat rests with the author of
individual board device tree files, and any requirements imposed by the
bindings for the individual client devices in use by that board, i.e. whether
they require certain specific named states for dynamic pin configuration.

== Pinctrl client devices ==

For each client device individually, every pin state is assigned an integer
ID. These numbers start at 0, and are contiguous. For each state ID, a unique
property exists to define the pin configuration. Each state may also be
assigned a name. When names are used, another property exists to map from
those names to the integer IDs.

Each client device's own binding determines the set of states that must be
defined in its device tree node, and whether to define the set of state
IDs that must be provided, or whether to define the set of state names that
must be provided.

Required properties:
pinctrl-0:	List of phandles, each pointing at a pin configuration
		node. These referenced pin configuration nodes must be child
		nodes of the pin controller that they configure. Multiple
		entries may exist in this list so that multiple pin
		controllers may be configured, or so that a state may be built
		from multiple nodes for a single pin controller, each
		contributing part of the overall configuration. See the next
		section of this document for details of the format of these
		pin configuration nodes.

		In some cases, it may be useful to define a state, but for it
		to be empty. This may be required when a common IP block is
		used in an SoC either without a pin controller, or where the
		pin controller does not affect the HW module in question. If
		the binding for that IP block requires certain pin states to
		exist, they must still be defined, but may be left empty.

Optional properties:
pinctrl-1:	List of phandles, each pointing at a pin configuration
		node within a pin controller.
...
pinctrl-n:	List of phandles, each pointing at a pin configuration
		node within a pin controller.
pinctrl-names:	The list of names to assign states. List entry 0 defines the
		name for integer state ID 0, list entry 1 for state ID 1, and
		so on.

For example:

	/* For a client device requiring named states */
	device {
		pinctrl-names = "active", "idle";
		pinctrl-0 = <&state_0_node_a>;
		pinctrl-1 = <&state_1_node_a &state_1_node_b>;
	};

	/* For the same device if using state IDs */
	device {
		pinctrl-0 = <&state_0_node_a>;
		pinctrl-1 = <&state_1_node_a &state_1_node_b>;
	};

	/*
	 * For an IP block whose binding supports pin configuration,
	 * but in use on an SoC that doesn't have any pin control hardware
	 */
	device {
		pinctrl-names = "active", "idle";
		pinctrl-0 = <>;
		pinctrl-1 = <>;
	};

== Pin controller devices ==
Required properties: See the pin controller driver specific documentation

Optional properties:
#pinctrl-cells:	Number of pin control cells in addition to the index within the
		pin controller device instance

Pin controller devices should contain the pin configuration nodes that client
devices reference.

For example:

	pincontroller {
		... /* Standard DT properties for the device itself elided */

		state_0_node_a {
			...
		};
		state_1_node_a {
			...
		};
		state_1_node_b {
			...
		};
	}

The contents of each of those pin configuration child nodes is defined
entirely by the binding for the individual pin controller device. There
exists no common standard for this content. The pinctrl framework only
provides generic helper bindings that the pin controller driver can use.

The pin configuration nodes need not be direct children of the pin controller
device; they may be grandchildren, for example. Whether this is legal, and
whether there is any interaction between the child and intermediate parent
nodes, is again defined entirely by the binding for the individual pin
controller device.

== Generic pin multiplexing node content ==

pin multiplexing nodes:

function		- the mux function to select
groups			- the list of groups to select with this function
			  (either this or "pins" must be specified)
pins			- the list of pins to select with this function (either
			  this or "groups" must be specified)

Example:

state_0_node_a {
	uart0 {
		function = "uart0";
		groups = "u0rxtx", "u0rtscts";
	};
};
state_1_node_a {
	spi0 {
		function = "spi0";
		groups = "spi0pins";
	};
};
state_2_node_a {
	function = "i2c0";
	pins = "mfio29", "mfio30";
};

Optionally an alternative binding can be used if more suitable depending on the
pin controller hardware. For hardware where there is a large number of identical
pin controller instances, naming each pin and function can easily become
unmaintainable. This is especially the case if the same controller is used for
different pins and functions depending on the SoC revision and packaging.

For cases like this, the pin controller driver may use pinctrl-pin-array helper
binding with a hardware based index and a number of pin configuration values:

pincontroller {
	... /* Standard DT properties for the device itself elided */
	#pinctrl-cells = <2>;

	state_0_node_a {
		pinctrl-pin-array = <
			0 A_DELAY_PS(0) G_DELAY_PS(120)
			4 A_DELAY_PS(0) G_DELAY_PS(360)
			...
		>;
	};
	...
};

Above #pinctrl-cells specifies the number of value cells in addition to the
index of the registers. This is similar to the interrupts-extended binding with
one exception. There is no need to specify the phandle for each entry as that
is already known as the defined pins are always children of the pin controller
node. Further having the phandle pointing to another pin controller would not
currently work as the pinctrl framework uses named modes to group pins for each
pin control device.

The index for pinctrl-pin-array must relate to the hardware for the pinctrl
registers, and must not be a virtual index of pin instances. The reason for
this is to avoid mapping of the index in the dts files and the pin controller
driver as it can change.

For hardware where pin multiplexing configurations have to be specified for
each single pin the number of required sub-nodes containing "pin" and
"function" properties can quickly escalate and become hard to write and
maintain.

For cases like this, the pin controller driver may use the pinmux helper
property, where the pin identifier is provided with mux configuration settings
in a pinmux group. A pinmux group consists of the pin identifier and mux
settings represented as a single integer or an array of integers.

The pinmux property accepts an array of pinmux groups, each of them describing
a single pin multiplexing configuration.

pincontroller {
	state_0_node_a {
		pinmux = <PINMUX_GROUP>, <PINMUX_GROUP>, ...;
	};
};

Each individual pin controller driver bindings documentation shall specify
how pin IDs and pin multiplexing configuration are defined and assembled
together in a pinmux group.

== Generic pin configuration node content ==

Many data items that are represented in a pin configuration node are common
and generic. Pin control bindings should use the properties defined below
where they are applicable; not all of these properties are relevant or useful
for all hardware or binding structures. Each individual binding document
should state which of these generic properties, if any, are used, and the
structure of the DT nodes that contain these properties.

Supported generic properties are:

pins			- the list of pins that properties in the node
			  apply to (either this, "group" or "pinmux" has to be
			  specified)
group			- the group to apply the properties to, if the driver
			  supports configuration of whole groups rather than
			  individual pins (either this, "pins" or "pinmux" has
			  to be specified)
pinmux			- the list of numeric pin ids and their mux settings
			  that properties in the node apply to (either this,
			  "pins" or "groups" have to be specified)
bias-disable		- disable any pin bias
bias-high-impedance	- high impedance mode ("third-state", "floating")
bias-bus-hold		- latch weakly
bias-pull-up		- pull up the pin
bias-pull-down		- pull down the pin
bias-pull-pin-default	- use pin-default pull state
drive-push-pull		- drive actively high and low
drive-open-drain	- drive with open drain
drive-open-source	- drive with open source
drive-strength		- sink or source at most X mA
input-enable		- enable input on pin (no effect on output, such as
			  enabling an input buffer)
input-disable		- disable input on pin (no effect on output, such as
			  disabling an input buffer)
input-schmitt-enable	- enable schmitt-trigger mode
input-schmitt-disable	- disable schmitt-trigger mode
input-debounce		- debounce mode with debound time X
power-source		- select between different power supplies
low-power-enable	- enable low power mode
low-power-disable	- disable low power mode
output-disable		- disable output on a pin (such as disable an output
			  buffer)
output-enable		- enable output on a pin without actively driving it
			  (such as enabling an output buffer)
output-low		- set the pin to output mode with low level
output-high		- set the pin to output mode with high level
sleep-hardware-state	- indicate this is sleep related state which will be programmed
			  into the registers for the sleep state.
slew-rate		- set the slew rate

For example:

state_0_node_a {
	cts_rxd {
		pins = "GPIO0_AJ5", "GPIO2_AH4"; /* CTS+RXD */
		bias-pull-up;
	};
};
state_1_node_a {
	rts_txd {
		pins = "GPIO1_AJ3", "GPIO3_AH3"; /* RTS+TXD */
		output-high;
	};
};
state_2_node_a {
	foo {
		group = "foo-group";
		bias-pull-up;
	};
};
state_3_node_a {
	mux {
		pinmux = <GPIOx_PINm_MUXn>, <GPIOx_PINj_MUXk)>;
		input-enable;
	};
};

Some of the generic properties take arguments. For those that do, the
arguments are described below.

- pins takes a list of pin names or IDs as a required argument. The specific
  binding for the hardware defines:
  - Whether the entries are integers or strings, and their meaning.

- pinmux takes a list of pin IDs and mux settings as required argument. The
  specific bindings for the hardware defines:
  - How pin IDs and mux settings are defined and assembled together in a single
    integer or an array of integers.

- bias-pull-up, -down and -pin-default take as optional argument on hardware
  supporting it the pull strength in Ohm. bias-disable will disable the pull.

- drive-strength takes as argument the target strength in mA.

- input-debounce takes the debounce time in usec as argument
  or 0 to disable debouncing

More in-depth documentation on these parameters can be found in
<include/linux/pinctrl/pinconf-generic.h>

One-register-per-pin type device tree based pinctrl driver

Required properties:
- compatible : "pinctrl-single" or "pinconf-single".
  "pinctrl-single" means that pinconf isn't supported.
  "pinconf-single" means that generic pinconf is supported.

- reg : offset and length of the register set for the mux registers

- #pinctrl-cells : number of cells in addition to the index, set to 1
  for pinctrl-single,pins and 2 for pinctrl-single,bits

- pinctrl-single,register-width : pinmux register access width in bits

- pinctrl-single,function-mask : mask of allowed pinmux function bits
  in the pinmux register

Optional properties:
- pinctrl-single,function-off : function off mode for disabled state if
  available and same for all registers; if not specified, disabling of
  pin functions is ignored

- pinctrl-single,bit-per-mux : boolean to indicate that one register controls
  more than one pin, for which "pinctrl-single,function-mask" property specifies
 position mask of pin.

- pinctrl-single,drive-strength : array of value that are used to configure
  drive strength in the pinmux register. They're value of drive strength
  current and drive strength mask.

		/* drive strength current, mask */
		pinctrl-single,power-source = <0x30 0xf0>;

- pinctrl-single,bias-pullup : array of value that are used to configure the
  input bias pullup in the pinmux register.

		/* input, enabled pullup bits, disabled pullup bits, mask */
		pinctrl-single,bias-pullup = <0 1 0 1>;

- pinctrl-single,bias-pulldown : array of value that are used to configure the
  input bias pulldown in the pinmux register.

		/* input, enabled pulldown bits, disabled pulldown bits, mask */
		pinctrl-single,bias-pulldown = <2 2 0 2>;

  * Two bits to control input bias pullup and pulldown: User should use
    pinctrl-single,bias-pullup & pinctrl-single,bias-pulldown. One bit means
    pullup, and the other one bit means pulldown.
  * Three bits to control input bias enable, pullup and pulldown. User should
    use pinctrl-single,bias-pullup & pinctrl-single,bias-pulldown. Input bias
    enable bit should be included in pullup or pulldown bits.
  * Although driver could set PIN_CONFIG_BIAS_DISABLE, there's no property as
    pinctrl-single,bias-disable. Because pinctrl single driver could implement
    it by calling pulldown, pullup disabled.

- pinctrl-single,input-schmitt : array of value that are used to configure
  input schmitt in the pinmux register. In some silicons, there're two input
  schmitt value (rising-edge & falling-edge) in the pinmux register.

		/* input schmitt value, mask */
		pinctrl-single,input-schmitt = <0x30 0x70>;

- pinctrl-single,input-schmitt-enable : array of value that are used to
  configure input schmitt enable or disable in the pinmux register.

		/* input, enable bits, disable bits, mask */
		pinctrl-single,input-schmitt-enable = <0x30 0x40 0 0x70>;

- pinctrl-single,low-power-mode : array of value that are used to configure
  low power mode of this pin. For some silicons, the low power mode will
  control the output of the pin when the pad including the pin enter low
  power mode.
		/* low power mode value, mask */
		pinctrl-single,low-power-mode = <0x288 0x388>;

- pinctrl-single,gpio-range : list of value that are used to configure a GPIO
  range. They're value of subnode phandle, pin base in pinctrl device, pin
  number in this range, GPIO function value of this GPIO range.
  The number of parameters is depend on #pinctrl-single,gpio-range-cells
  property.

		/* pin base, nr pins & gpio function */
		pinctrl-single,gpio-range = <&range 0 3 0 &range 3 9 1>;

- interrupt-controller : standard interrupt controller binding if using
  interrupts for wake-up events for example. In this case pinctrl-single
  is set up as a chained interrupt controller and the wake-up interrupts
  can be requested by the drivers using request_irq().

- #interrupt-cells : standard interrupt binding if using interrupts

This driver assumes that there is only one register for each pin (unless the
pinctrl-single,bit-per-mux is set), and uses the common pinctrl bindings as
specified in the pinctrl-bindings.txt document in this directory.

The pin configuration nodes for pinctrl-single are specified as pinctrl
register offset and value pairs using pinctrl-single,pins. Only the bits
specified in pinctrl-single,function-mask are updated. For example, setting
a pin for a device could be done with:

	pinctrl-single,pins = <0xdc 0x118>;

Where 0xdc is the offset from the pinctrl register base address for the
device pinctrl register, and 0x118 contains the desired value of the
pinctrl register. See the device example and static board pins example
below for more information.

In case when one register changes more than one pin's mux the
pinctrl-single,bits need to be used which takes three parameters:

	pinctrl-single,bits = <0xdc 0x18 0xff>;

Where 0xdc is the offset from the pinctrl register base address for the
device pinctrl register, 0x18 is the desired value, and 0xff is the sub mask to
be used when applying this change to the register.


Optional sub-node: In case some pins could be configured as GPIO in the pinmux
register, those pins could be defined as a GPIO range. This sub-node is required
by pinctrl-single,gpio-range property.

Required properties in sub-node:
- #pinctrl-single,gpio-range-cells : the number of parameters after phandle in
  pinctrl-single,gpio-range property.

	range: gpio-range {
		#pinctrl-single,gpio-range-cells = <3>;
	};


Example:

/* SoC common file */

/* first controller instance for pins in core domain */
pmx_core: pinmux@4a100040 {
	compatible = "pinctrl-single";
	reg = <0x4a100040 0x0196>;
	#address-cells = <1>;
	#size-cells = <0>;
	#interrupt-cells = <1>;
	interrupt-controller;
	pinctrl-single,register-width = <16>;
	pinctrl-single,function-mask = <0xffff>;
};

/* second controller instance for pins in wkup domain */
pmx_wkup: pinmux@4a31e040 {
	compatible = "pinctrl-single";
	reg = <0x4a31e040 0x0038>;
	#address-cells = <1>;
	#size-cells = <0>;
	#interrupt-cells = <1>;
	interrupt-controller;
	pinctrl-single,register-width = <16>;
	pinctrl-single,function-mask = <0xffff>;
};

control_devconf0: pinmux@48002274 {
	compatible = "pinctrl-single";
	reg = <0x48002274 4>;	/* Single register */
	#address-cells = <1>;
	#size-cells = <0>;
	pinctrl-single,bit-per-mux;
	pinctrl-single,register-width = <32>;
	pinctrl-single,function-mask = <0x5F>;
};

/* third controller instance for pins in gpio domain */
pmx_gpio: pinmux@d401e000 {
	compatible = "pinconf-single";
	reg = <0xd401e000 0x0330>;
	#address-cells = <1>;
	#size-cells = <1>;
	ranges;

	pinctrl-single,register-width = <32>;
	pinctrl-single,function-mask = <7>;

	/* sparse GPIO range could be supported */
	pinctrl-single,gpio-range = <&range 0 3 0 &range 3 9 1
				&range 12 1 0 &range 13 29 1
				&range 43 1 0 &range 44 49 1
				&range 94 1 1 &range 96 2 1>;

	range: gpio-range {
		#pinctrl-single,gpio-range-cells = <3>;
	};
};


/* board specific .dts file */

&pmx_core {

	/*
	 * map all board specific static pins enabled by the pinctrl driver
	 * itself during the boot (or just set them up in the bootloader)
	 */
	pinctrl-names = "default";
	pinctrl-0 = <&board_pins>;

	board_pins: pinmux_board_pins {
		pinctrl-single,pins = <
			0x6c 0xf
			0x6e 0xf
			0x70 0xf
			0x72 0xf
		>;
	};

	uart0_pins: pinmux_uart0_pins {
		pinctrl-single,pins = <
			0x208 0		/* UART0_RXD (IOCFG138) */
			0x20c 0		/* UART0_TXD (IOCFG139) */
		>;
		pinctrl-single,bias-pulldown = <0 2 2>;
		pinctrl-single,bias-pullup = <0 1 1>;
	};

	/* map uart2 pins */
	uart2_pins: pinmux_uart2_pins {
		pinctrl-single,pins = <
			0xd8 0x118
			0xda 0
			0xdc 0x118
			0xde 0
		>;
	};
};

&control_devconf0 {
	mcbsp1_pins: pinmux_mcbsp1_pins {
		pinctrl-single,bits = <
			0x00 0x18 0x18 /* FSR/CLKR signal from FSX/CLKX pin */
		>;
	};

	mcbsp2_clks_pins: pinmux_mcbsp2_clks_pins {
		pinctrl-single,bits = <
			0x00 0x40 0x40 /* McBSP2 CLKS from McBSP_CLKS pin */
		>;
	};

};

&uart1 {
       pinctrl-names = "default";
       pinctrl-0 = <&uart0_pins>;
};

&uart2 {
       pinctrl-names = "default";
       pinctrl-0 = <&uart2_pins>;
};

OMAP Pinctrl definitions

Required properties:
- compatible : Should be one of:
  "ti,omap2420-padconf" - OMAP2420 compatible pinctrl
  "ti,omap2430-padconf" - OMAP2430 compatible pinctrl
  "ti,omap3-padconf" - OMAP3 compatible pinctrl
  "ti,omap4-padconf" - OMAP4 compatible pinctrl
  "ti,omap5-padconf" - OMAP5 compatible pinctrl
  "ti,dra7-padconf" - DRA7 compatible pinctrl
  "ti,am437-padconf" - AM437x compatible pinctrl

See Documentation/devicetree/bindings/pinctrl/pinctrl-single.txt for further details.
