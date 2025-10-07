#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/iio/iio.h>
#include <linux/math64.h>

static u64 pocket_geiger_get_usvh(void);
static u64 pocket_geiger_get_usvh_error(void);

static int pocket_geiger_read_raw(struct iio_dev *indio_dev, struct iio_chan_spec const *chan, int *val, int *val2, long mask) {
	switch (mask) {
		case IIO_CHAN_INFO_RAW:
			switch (chan->channel) {
				case 0: 
					u64 radiation_raw_val = pocket_geiger_get_usvh();
					*val = radiation_raw_val / 1000;
					*val2 = (radiation_raw_val % 1000) * 1000;
					break;
				
				case 1: 
					u64 err_raw_val = pocket_geiger_get_usvh_error();
					*val = err_raw_val / 1000;
					*val2 = (err_raw_val % 1000) * 1000;
					break;
				
				default:
					return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
	}
	return IIO_VAL_INT_PLUS_MICRO;
}

// -----------------------------------------------------------------------------
//                       Local Variables
// -----------------------------------------------------------------------------

// Radiation and noise count
static u32 radiation_count        = 0;
static u32 noise_count            = 0;
static u8  history_length         = 0;
static u8  history_index          = 0;
static u32 count                  = 0;
static u32 prev_jiffy             = 0;
static u32 previous_history_jiffy = 0;
static u32 k_alpha                = 53032; // multiply by 1000

struct task_struct *process_thread;

struct iio_dev *pocket_geiger;

struct pocket_geiger_data_t {
	u32 dt_process_period;
	u32 dt_history_length;
	u32 dt_history_unit;
	int ns_irq;
	int sig_irq;
};

static struct pocket_geiger_data_t pocket_geiger_data;

static struct of_device_id pocket_geiger_of_match[] = {
	{.compatible = "Radiation Watch,The Type 5 Pocket Geiger",},
	{},
};
MODULE_DEVICE_TABLE(of, pocket_geiger_of_match);

static const struct iio_info pocket_geiger_info = {
	.read_raw = pocket_geiger_read_raw,
};

static const struct iio_chan_spec pocket_geiger_channels[2] = {
	{
		.type = IIO_COUNT,
		.channel = 0,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.extend_name = "radiation",
	},
	{
		.type = IIO_COUNT,
		.channel = 1,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
		.extend_name = "error",

	},
};

// -----------------------------------------------------------------------------
//                       Local Function
// -----------------------------------------------------------------------------

static irqreturn_t pocket_geiger_irq_cb(int irq,void *dev_id) {
	if (irq == pocket_geiger_data.ns_irq) {
		noise_count++;
	}
	if (irq == pocket_geiger_data.sig_irq) {
		radiation_count++;
	}
	return IRQ_HANDLED;
}

static int pocket_geiger_process(void *data)
{
	struct pocket_geiger_data_t *param = (struct pocket_geiger_data_t *)data;
	static u32 count_history[100];

	pr_info("%s: started radiation process...\n", THIS_MODULE->name);
	while (!kthread_should_stop()) {
		u32 curr_jiffy = get_jiffies_64();
		u32 curr_noise = 0;
		u32 curr_radiation = 0;
		if (jiffies_to_msecs(curr_jiffy - prev_jiffy) >= param->dt_process_period) {
			// Disable Interrupt to store the current radiation count.
			disable_irq(pocket_geiger_data.ns_irq);
			disable_irq(pocket_geiger_data.sig_irq);

			// Store radiation and noise count.
			curr_noise = noise_count;
			curr_radiation = radiation_count;

			// Reset radiation and noise count.
			radiation_count = 0;
			noise_count = 0;

			// Enable Interrupt to continue receiving signal.
			enable_irq(pocket_geiger_data.ns_irq);
			enable_irq(pocket_geiger_data.sig_irq);

			if (noise_count == 0) {
				// Store count into count history array.
				count_history[history_index] = curr_radiation;

				// Add current radiation count to total count.
				count += curr_radiation;
			}

			// Shift the value of history count array to the left for each 6 seconds.
			if (jiffies_to_msecs(curr_jiffy - previous_history_jiffy) >= param->dt_history_unit * 1000) {
				previous_history_jiffy += msecs_to_jiffies(param->dt_history_unit * 1000);
				history_index = (history_index + 1) % param->dt_history_length;
				if (history_length < (param->dt_history_length - 1)) {
					history_length++;
				}
				count -= count_history[history_index];
				count_history[history_index] = 0;
			}

			prev_jiffy = curr_jiffy;
		}
	}
	return 0;
}

static u32 pocket_geiger_get_radiation_count(void) { return count; }

static u32 pocket_geiger_get_integration_time(void)
{
  	return jiffies_to_msecs(history_length * pocket_geiger_data.dt_history_unit * 1000 + prev_jiffy - previous_history_jiffy);
}

static u64 pocket_geiger_get_usvh(void) {
	// cpm = uSv x alpha
	u32 time = pocket_geiger_get_integration_time();
	// pr_info("%llu", (u64)pocket_geiger_get_radiation_count() * 60000 * 1000 * 1000 / (time * k_alpha));
	return (time > 0) ? (u64)pocket_geiger_get_radiation_count() * 60000 * 1000 * 1000 / (time * k_alpha) : 0;
}

static u64 pocket_geiger_get_usvh_error(void) {
	u32 time = pocket_geiger_get_integration_time();
  	return (time > 0) ? (u64)int_sqrt64(pocket_geiger_get_radiation_count() * 60000 * 1000 * 1000000 / (time * k_alpha)) : 0;
}

/**************************************************************************//**
 *  Pocket Geiger probe.
 *****************************************************************************/
static int pocket_geiger_probe(struct platform_device *pdev) {
	struct gpio_desc *noise;
	struct gpio_desc *sig;
	int err;
	
	// get sensor configuration
	if (!of_property_present(pdev->dev.of_node, "process-period")) pr_err("%s have no process-period prop\n", THIS_MODULE->name);
	if (!of_property_present(pdev->dev.of_node, "history-length")) pr_err("%s have no history-length value\n", THIS_MODULE->name);
	if (!of_property_present(pdev->dev.of_node, "history-unit")) pr_err("%s have no history-unit value\n", THIS_MODULE->name);
	
	of_property_read_u32(pdev->dev.of_node, "process-period", &pocket_geiger_data.dt_process_period);
	of_property_read_u32(pdev->dev.of_node, "history-length", &pocket_geiger_data.dt_history_length);
	of_property_read_u32(pdev->dev.of_node, "history-unit", &pocket_geiger_data.dt_history_unit);

	// get sensor gpios from DT
	noise = devm_gpiod_get(&pdev->dev, "ns", GPIOD_IN);
	if (IS_ERR(noise)) {
		pr_err("%s: failed to get noise GPIO\n", THIS_MODULE->name);
		return PTR_ERR(noise);
	}

	sig = devm_gpiod_get(&pdev->dev, "sig", GPIOD_IN);
	if (IS_ERR(sig)) {
		pr_err("%s failed to get signal GPIO\n", THIS_MODULE->name);
		return PTR_ERR(sig);
	}
	
	// config gpios as input 
	err = gpiod_direction_input(noise);
	if (err < 0) { 
		pr_err("%s: noise GPIO set as input failed (err = %d)\n", THIS_MODULE->name, err);
		return err;
	}
	err = gpiod_direction_input(sig);
	if (err < 0) { 
		pr_err("%s: sig GPIO set as input failed (err = %d)\n", THIS_MODULE->name, err);
		return err;
	}
	
	// allocate device as iio device
	pocket_geiger = devm_iio_device_alloc(&pdev->dev, sizeof(pocket_geiger_data));
	if (!pocket_geiger)
		return -ENOMEM;
	
	// config device 
	pocket_geiger->name = "pocket_geiger";
	pocket_geiger->info = &pocket_geiger_info;
	pocket_geiger->modes = INDIO_DIRECT_MODE;
	pocket_geiger->channels = pocket_geiger_channels;
	pocket_geiger->num_channels = sizeof(pocket_geiger_channels) / sizeof(struct iio_chan_spec);
	
	// config gpio irq's handlers
	pocket_geiger_data.ns_irq = gpiod_to_irq(noise);
	pocket_geiger_data.sig_irq = gpiod_to_irq(sig);

	err = devm_request_irq(&pdev->dev,
						   pocket_geiger_data.ns_irq,
						   pocket_geiger_irq_cb,
		  				   IRQF_TRIGGER_FALLING,
		  				   pocket_geiger->name,
		  				   NULL);    
	if (err < 0) {
		pr_err("%s: register noise irq failed (err = %d)\n", THIS_MODULE->name, err);
		return err;
	}
	err = devm_request_irq(&pdev->dev,
						   pocket_geiger_data.sig_irq,
						   pocket_geiger_irq_cb,
		  				   IRQF_TRIGGER_FALLING,
		  				   pocket_geiger->name,
		  				   NULL); 
	if (err) {
		pr_err("%s: register radiation irq failed (err = %d)\n", THIS_MODULE->name, err); 
		return err;
	}

	// register device as iio device
	err = devm_iio_device_register(&pdev->dev, pocket_geiger);
	if (err) {
		pr_err("%s: failed to register as iio device\n", THIS_MODULE->name);
		return err;
	}

	// create and run radiation process 
	process_thread = kthread_run(pocket_geiger_process, &pocket_geiger_data, "radiation process thread");

	pr_info("%s: loaded to kernel\n", THIS_MODULE->name);
    return 0;
}

static void pocket_geiger_remove(struct platform_device *pdev) {
	int err;

	err = kthread_stop(process_thread);
	if (err != 0) 
		pr_err("%s: stop radiation process failed\n", THIS_MODULE->name);
	else
		pr_info("%s: stopped radiation process\n", THIS_MODULE->name);

	pr_info("%s: unloaded from kernel\n", THIS_MODULE->name);
}

static struct platform_driver pocket_geiger_driver = {
	.probe = pocket_geiger_probe,
    .remove = pocket_geiger_remove,
    .driver = {
		.owner = THIS_MODULE,
		.name = "Pocket Geiger",
		.of_match_table = pocket_geiger_of_match,
	},
};

module_platform_driver(pocket_geiger_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TungDT53");
MODULE_DESCRIPTION("Pocket Geiger Radiation Device Driver");
