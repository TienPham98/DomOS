import asyncio
import logging
from amqtt.broker import Broker

logger = logging.getLogger("domos.mqtt_broker")

config = {
    'listeners': {
        'default': {
            'type': 'tcp',
            'bind': '0.0.0.0:1883',
            'max_connections': 100,
        }
    },
    'sys_interval': 20,
    'auth': {
        'allow-anonymous': True,
    }
}

async def start_broker():
    broker = Broker(config)
    await broker.start()
    logger.info("DomOS MQTT Broker started on 0.0.0.0:1883")
    try:
        while True:
            await asyncio.sleep(3600)
    except asyncio.CancelledError:
        await broker.shutdown()

if __name__ == '__main__':
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(name)s: %(message)s"
    )
    asyncio.run(start_broker())
