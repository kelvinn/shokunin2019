import csv
from pubnub.callbacks import SubscribeCallback
from pubnub.enums import PNStatusCategory
from pubnub.pnconfiguration import PNConfiguration
from pubnub.pubnub import PubNub
 
pnconfig = PNConfiguration()
 
pnconfig.subscribe_key = 'sub-c-5f1b7c8e-fbee-11e3-aa40-02ee2ddab7fe'

LOCATIONS = {}

pubnub = PubNub(pnconfig)


def write_message_to_csv(message):

    with open('data.csv', 'a') as csvfile:
        sensorwriter = csv.writer(csvfile, delimiter=',', quotechar='|', quoting=csv.QUOTE_MINIMAL)
        sensorwriter.writerow(message.values())
 
 
class MySubscribeCallback(SubscribeCallback):
    def presence(self, pubnub, presence):
        pass  # handle incoming presence data
 
    def status(self, pubnub, status):
        if status.category == PNStatusCategory.PNUnexpectedDisconnectCategory:
            print('PNUnexpectedDisconnectCategory')
 
        elif status.category == PNStatusCategory.PNConnectedCategory:
            # Connect event. You can do stuff like publish, and know you'll get it.
            # Or just use the connected event to confirm you are subscribed for
            # UI / internal notifications, etc
            print('PNConnectedCategory')

        elif status.category == PNStatusCategory.PNReconnectedCategory:
            print('PNReconnectedCategory')

        elif status.category == PNStatusCategory.PNDecryptionErrorCategory:
            print('PNDecryptionErrorCategory')
            # Handle message decryption error. Probably client configured to
            # encrypt messages and on live data feed it received plain text.
 
    def message(self, pubnub, message):
        print(message.message)
        write_message_to_csv(message.message)
 
 
pubnub.add_listener(MySubscribeCallback())
pubnub.subscribe().channels('pubnub-sensor-network').execute()