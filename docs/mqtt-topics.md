# MQTT Topics

Base: \intercom/\

| Topic                     | Payload         | Notes                    |
|--------------------------|-----------------|--------------------------|
| intercom/availability    | online / offline (retained) | Device availability |
| intercom/button/1..12    | ON / OFF        | One topic per button     |
