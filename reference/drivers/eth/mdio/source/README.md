# Source tham khảo — MDIO

## Primary reference

- [davinci_mdio.c (mainline)](https://github.com/torvalds/linux/blob/master/drivers/net/ethernet/ti/davinci_mdio.c) — TI DaVinci MDIO bus driver, dùng trên AM335x

## Đọc những gì

- `davinci_mdio_probe()` — clock enable, divisor tính từ bus_freq/phyclk, MDIO_CONTROL write
- `davinci_mdio_read()` / `davinci_mdio_write()` — USERACCESS0 register, PHYREG + PHYADR fields, poll GO bit
- `struct davinci_mdio_data` — state struct: base, clk, bus_freq

## Ghi chú

- NothanOS không dùng Device Tree → bỏ `of_match_table`, hardcode divisor từ TRM §14.4.3
- Không có `devm_*` allocator → dùng static struct
- Pattern reference: Linux `drivers/net/ethernet/ti/davinci_mdio.c` — re-implemented
