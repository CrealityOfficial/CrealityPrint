
window.configAlpha = {
    china: {
        apiAddress: 'https://admin-pre.crealitycloud.cn',
        websiteAddress: 'https://pre.crealitycloud.cn'
    },
    oversea: {
        apiAddress: 'https://admin-pre.crealitycloud.com',
        websiteAddress: 'https://pre.crealitycloud.com',
        accountCenter: {
            address: 'https://id-pre.creality.com/connect',
            client_id: '275d77a3cc5059089a6d39c9292de38b',
            redirect_uri: 'https://admin-pre.crealitycloud.com/oauth'
        }
    },
    modeFe: 'build'
};

window.configBeta = {
    china: {
        apiAddress: 'https://api.crealitycloud.cn',
        websiteAddress: 'https://www.crealitycloud.cn'
    },
    oversea: {
        apiAddress: 'https://api.crealitycloud.com',
        websiteAddress: 'https://www.crealitycloud.com',
        accountCenter: {
            address: 'https://id.creality.com',
            client_id: 'f9c302ecc29c59a0a6e921ff39a073ca',
            redirect_uri: 'https://www.crealitycloud.com/oauth'
        }
    },
    modeFe: 'build'
};

window.configRelease = window.configBeta;
