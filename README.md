# node-alt-config
NodeJS native bindings (NAPI) for alt-config

# Example

```js
const altConfig = require('alt-config')

altConfig.parse(`a: "b",
c: {
  d: 5,
  e: [
    5,
    6,
    "s"
  ]
  f: true,
}`)
  .then(config => {
    console.log(config)
  })
  .catch(err => {
    console.error(err)
  })
```